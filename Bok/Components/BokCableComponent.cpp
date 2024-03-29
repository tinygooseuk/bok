// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. 

#include "BokCableComponent.h"
#include "EngineGlobals.h"
#include "PrimitiveViewRelevance.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "WorldCollision.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "SceneManagement.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "Engine/Engine.h"
#include "BokCableComponentStats.h"
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"

DECLARE_CYCLE_STAT(TEXT("Cable Sim"), STAT_BokCable_SimTime, STATGROUP_BokCableComponent);
DECLARE_CYCLE_STAT(TEXT("Cable Solve"), STAT_BokCable_SolveTime, STATGROUP_BokCableComponent);
DECLARE_CYCLE_STAT(TEXT("Cable Collision"), STAT_BokCable_CollisionTime, STATGROUP_BokCableComponent);
DECLARE_CYCLE_STAT(TEXT("Cable Integrate"), STAT_BokCable_IntegrateTime, STATGROUP_BokCableComponent);

static FName CableEndSocketName(TEXT("CableEnd"));
static FName CableStartSocketName(TEXT("CableStart"));

//////////////////////////////////////////////////////////////////////////

/** Index Buffer */
class FBokCableIndexBuffer : public FIndexBuffer 
{
public:
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), NumIndices * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	int32 NumIndices;
};

/** Dynamic data sent to render thread */
struct FBokCableDynamicData
{
	/** Array of points */
	TArray<FVector> CablePoints;
};

//////////////////////////////////////////////////////////////////////////
// FBokCableSceneProxy

class FBokCableSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FBokCableSceneProxy(UBokCableComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Material(NULL)
		, VertexFactory(GetScene().GetFeatureLevel(), "FBokCableSceneProxy")
		, DynamicData(NULL)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, NumSegments(Component->NumSegments)
		, CableWidth(Component->CableWidth)
		, NumSides(Component->NumSides)
		, TileMaterial(Component->TileMaterial)
	{
		VertexBuffers.InitWithDummyData(&VertexFactory, GetRequiredVertexCount());

		IndexBuffer.NumIndices = GetRequiredIndexCount();

		// Enqueue initialization of render resource
		BeginInitResource(&IndexBuffer);

		// Grab material
		Material = Component->GetMaterial(0);
		if(Material == NULL)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}

	virtual ~FBokCableSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();

		if(DynamicData != NULL)
		{
			delete DynamicData;
		}
	}

	int32 GetRequiredVertexCount() const
	{
		return (NumSegments + 1) * (NumSides + 1);
	}

	int32 GetRequiredIndexCount() const
	{
		return (NumSegments * NumSides * 2) * 3;
	}

	int32 GetVertIndex(int32 AlongIdx, int32 AroundIdx) const
	{
		return (AlongIdx * (NumSides+1)) + AroundIdx;
	}

	void BuildCableMesh(const TArray<FVector>& InPoints, TArray<FDynamicMeshVertex>& OutVertices, TArray<int32>& OutIndices)
	{
		const FColor VertexColor(255,255,255);
		const int32 NumPoints = InPoints.Num();
		const int32 SegmentCount = NumPoints-1;

		// Build vertices

		// We double up the first and last vert of the ring, because the UVs are different
		int32 NumRingVerts = NumSides+1;

		// For each point along spline..
		for(int32 PointIdx=0; PointIdx<NumPoints; PointIdx++)
		{
			const float AlongFrac = (float)PointIdx/(float)SegmentCount; // Distance along cable

			// Find direction of cable at this point, by averaging previous and next points
			const int32 PrevIndex = FMath::Max(0, PointIdx-1);
			const int32 NextIndex = FMath::Min(PointIdx+1, NumPoints-1);
			const FVector ForwardDir = (InPoints[NextIndex] - InPoints[PrevIndex]).GetSafeNormal();

			// Find quat from up (Z) vector to forward
			const FQuat DeltaQuat = FQuat::FindBetween(FVector(0, 0, -1), ForwardDir);

			// Apply quat orth vectors
			const FVector RightDir = DeltaQuat.RotateVector(FVector(0, 1, 0));
			const FVector UpDir = DeltaQuat.RotateVector(FVector(1, 0, 0));

			// Generate a ring of verts
			for(int32 VertIdx = 0; VertIdx<NumRingVerts; VertIdx++)
			{
				const float AroundFrac = float(VertIdx)/float(NumSides);
				// Find angle around the ring
				const float RadAngle = 2.f * PI * AroundFrac;
				// Find direction from center of cable to this vertex
				const FVector OutDir = (FMath::Cos(RadAngle) * UpDir) + (FMath::Sin(RadAngle) * RightDir);

				FDynamicMeshVertex Vert;
				Vert.Position = InPoints[PointIdx] + (OutDir * 0.5f * CableWidth);
				Vert.TextureCoordinate[0] = FVector2D(AlongFrac * TileMaterial, AroundFrac);
				Vert.Color = VertexColor;
				Vert.SetTangents(ForwardDir, OutDir ^ ForwardDir, OutDir);
				OutVertices.Add(Vert);
			}
		}

		// Build triangles
		for(int32 SegIdx=0; SegIdx<SegmentCount; SegIdx++)
		{
			for(int32 SideIdx=0; SideIdx<NumSides; SideIdx++)
			{
				int32 TL = GetVertIndex(SegIdx, SideIdx);
				int32 BL = GetVertIndex(SegIdx, SideIdx+1);
				int32 TR = GetVertIndex(SegIdx+1, SideIdx);
				int32 BR = GetVertIndex(SegIdx+1, SideIdx+1);

				OutIndices.Add(TL);
				OutIndices.Add(BL);
				OutIndices.Add(TR);

				OutIndices.Add(TR);
				OutIndices.Add(BL);
				OutIndices.Add(BR);
			}
		}
	}

	/** Called on render thread to assign new dynamic data */
	void SetDynamicData_RenderThread(FBokCableDynamicData* NewDynamicData)
	{
		check(IsInRenderingThread());

		// Free existing data if present
		if(DynamicData)
		{
			delete DynamicData;
			DynamicData = NULL;
		}
		DynamicData = NewDynamicData;

		// Build mesh from cable points
		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		BuildCableMesh(NewDynamicData->CablePoints, Vertices, Indices);

		check(Vertices.Num() == GetRequiredVertexCount());
		check(Indices.Num() == GetRequiredIndexCount());

		for (int i = 0; i < Vertices.Num(); i++)
		{
			const FDynamicMeshVertex& Vertex = Vertices[i];

			VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
			VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector());
			VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
			VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
		}

		{
			auto& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.ColorVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
			RHIUnlockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
			RHIUnlockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
		}

		void* IndexBufferData = RHILockIndexBuffer(IndexBuffer.IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(IndexBufferData, &Indices[0], Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBuffer.IndexBufferRHI);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_BokCableSceneProxy_GetDynamicMeshElements );

		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if(bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy();
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				// Draw the mesh.
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;

				bool bHasPrecomputedVolumetricLightmap;
				FMatrix PreviousLocalToWorld;
				int32 SingleCaptureIndex;
				GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex);

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, UseEditorDepthTest());
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = GetRequiredIndexCount()/3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = GetRequiredVertexCount();
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
#endif
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual uint32 GetMemoryFootprint( void ) const override { return( sizeof( *this ) + GetAllocatedSize() ); }

	uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

private:

	UMaterialInterface* Material;
	FStaticMeshVertexBuffers VertexBuffers;
	FBokCableIndexBuffer IndexBuffer;
	FLocalVertexFactory VertexFactory;

	FBokCableDynamicData* DynamicData;

	FMaterialRelevance MaterialRelevance;

	int32 NumSegments;

	float CableWidth;

	int32 NumSides;

	float TileMaterial;
};



//////////////////////////////////////////////////////////////////////////

UBokCableComponent::UBokCableComponent( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	bAttachStart = true;
	bAttachEnd = true;
	CableWidth = 10.f;
	NumSegments = 10;
	NumSides = 4;
	EndLocation = FVector(100.f,0,0);
	CableLength = 100.f;
	SubstepTime = 0.02f;
	SolverIterations = 1;
	TileMaterial = 1.f;
	CollisionFriction = 0.2f;
	CableGravityScale = 1.f;

	SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
}

FPrimitiveSceneProxy* UBokCableComponent::CreateSceneProxy()
{
	return new FBokCableSceneProxy(this);
}

int32 UBokCableComponent::GetNumMaterials() const
{
	return 1;
}

void UBokCableComponent::OnRegister()
{
	Super::OnRegister();

	ResetCable();
}

void UBokCableComponent::VerletIntegrate(float InSubstepTime, const FVector& Gravity)
{
	SCOPE_CYCLE_COUNTER(STAT_BokCable_IntegrateTime);

	const int32 NumParticles = NumSegments+1;
	const float SubstepTimeSqr = InSubstepTime * InSubstepTime;

	for(int32 ParticleIdx=0; ParticleIdx<NumParticles; ParticleIdx++)
	{
		FBokCableParticle& Particle = Particles[ParticleIdx];
		if(Particle.bFree)
		{
			// Calc overall force
			const FVector ParticleForce = Gravity + CableForce;

			// Find vel
			const FVector Vel = Particle.Position - Particle.OldPosition;
			// Update position
			const FVector NewPosition = Particle.Position + Vel + (SubstepTimeSqr * ParticleForce);

			Particle.OldPosition = Particle.Position;
			Particle.Position = NewPosition;
		}
	}
}

/** Solve a single distance constraint between a pair of particles */
static void SolveDistanceConstraint(FBokCableParticle& ParticleA, FBokCableParticle& ParticleB, float DesiredDistance)
{
	// Find current vector between particles
	FVector Delta = ParticleB.Position - ParticleA.Position;
	// 
	float CurrentDistance = Delta.Size();
	float ErrorFactor = (CurrentDistance - DesiredDistance)/CurrentDistance;

	// Only move free particles to satisfy constraints
	if(ParticleA.bFree && ParticleB.bFree)
	{
		ParticleA.Position += ErrorFactor * 0.5f * Delta;
		ParticleB.Position -= ErrorFactor * 0.5f * Delta;
	}
	else if(ParticleA.bFree)
	{
		ParticleA.Position += ErrorFactor * Delta;
	}
	else if(ParticleB.bFree)
	{
		ParticleB.Position -= ErrorFactor * Delta;
	}
}

void UBokCableComponent::SolveConstraints()
{
	SCOPE_CYCLE_COUNTER(STAT_BokCable_SolveTime);

	const float SegmentLength = CableLength/(float)NumSegments;

	// For each iteration..
	for (int32 IterationIdx = 0; IterationIdx < SolverIterations; IterationIdx++)
	{
		// Solve distance constraint for each segment
		for (int32 SegIdx = 0; SegIdx < NumSegments; SegIdx++)
		{
			FBokCableParticle& ParticleA = Particles[SegIdx];
			FBokCableParticle& ParticleB = Particles[SegIdx + 1];
			// Solve for this pair of particles
			SolveDistanceConstraint(ParticleA, ParticleB, SegmentLength);
		}

		// If desired, solve stiffness constraints (distance constraints between every other particle)
		if (bEnableStiffness)
		{
			for (int32 SegIdx = 0; SegIdx < NumSegments-1; SegIdx++)
			{
				FBokCableParticle& ParticleA = Particles[SegIdx];
				FBokCableParticle& ParticleB = Particles[SegIdx + 2];
				SolveDistanceConstraint(ParticleA, ParticleB, 2.f*SegmentLength);
			}
		}
	}
}

void UBokCableComponent::PerformCableCollision()
{
	SCOPE_CYCLE_COUNTER(STAT_BokCable_CollisionTime);

	UWorld* World = GetWorld();
	// If we have a world, and collision is not disabled
	if (World && GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		// Get collision settings from component
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CableCollision));

		ECollisionChannel TraceChannel = GetCollisionObjectType();
		FCollisionResponseParams ResponseParams(GetCollisionResponseToChannels());

		// Iterate over each particle
		for (int32 ParticleIdx = 0; ParticleIdx < Particles.Num(); ParticleIdx++)
		{
			FBokCableParticle& Particle = Particles[ParticleIdx];
			// If particle is free
			if (Particle.bFree)
			{
				// Do sphere sweep
				FHitResult Result;
				bool bHit = World->SweepSingleByChannel(Result, Particle.OldPosition, Particle.Position, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(0.5f * CableWidth), Params, ResponseParams);
				// If we got a hit, resolve it
				if (bHit)
				{
					if (Result.bStartPenetrating)
					{
						Particle.Position += (Result.Normal * Result.PenetrationDepth);
					}
					else
					{
						Particle.Position = Result.Location;
					}

					// Find new velocity, after fixing collision
					FVector Delta = Particle.Position - Particle.OldPosition;
					// Find component in normal
					float NormalDelta = Delta | Result.Normal;
					// Find component in plane
					FVector PlaneDelta = Delta - (NormalDelta * Result.Normal);

					// Zero out any positive separation velocity, basically zero restitution
					Particle.OldPosition += (NormalDelta * Result.Normal);

					// Apply friction in plane of collision if desired
					if (CollisionFriction > KINDA_SMALL_NUMBER)
					{
						// Scale plane delta  by 'friction'
						FVector ScaledPlaneDelta = PlaneDelta * CollisionFriction;

						// Apply delta to old position reduce implied velocity in collision plane
						Particle.OldPosition += ScaledPlaneDelta;
					}
				}
			}
		}
	}
}

void UBokCableComponent::PerformSubstep(float InSubstepTime, const FVector& Gravity)
{
	SCOPE_CYCLE_COUNTER(STAT_BokCable_SimTime);

	VerletIntegrate(InSubstepTime, Gravity);

	SolveConstraints();

	if (bEnableCollision)
	{
		PerformCableCollision();
	}
}

void UBokCableComponent::ResetCable()
{
	const int32 NumParticles = NumSegments + 1;

	Particles.Reset();
	Particles.AddUninitialized(NumParticles);

	FVector CableStart, CableEnd;
	GetEndPositions(CableStart, CableEnd);

	const FVector Delta = CableEnd - CableStart;

	for (int32 ParticleIdx = 0; ParticleIdx < NumParticles; ParticleIdx++)
	{
		FBokCableParticle& Particle = Particles[ParticleIdx];

		const float Alpha = (float)ParticleIdx / (float)NumSegments;
		const FVector InitialPosition = CableStart + (Alpha * Delta);

		Particle.Position = InitialPosition;
		Particle.OldPosition = InitialPosition;
		Particle.bFree = true; // default to free, will be fixed if desired in TickComponent
	}
}

void UBokCableComponent::SetAttachEndToComponent(USceneComponent* Component, FName SocketName)
{
	AttachEndTo.OtherActor = Component->GetOwner();
	AttachEndTo.ComponentProperty = NAME_None;
	AttachEndTo.OverrideComponent = Component;
	AttachEndToSocketName = SocketName;
}

void UBokCableComponent::SetAttachEndTo(AActor* Actor, FName ComponentProperty, FName SocketName)
{
	AttachEndTo.OtherActor = Actor;
	AttachEndTo.ComponentProperty = ComponentProperty;
	AttachEndToSocketName = SocketName;
}

AActor* UBokCableComponent::GetAttachedActor() const
{
	return AttachEndTo.OtherActor;
}

USceneComponent* UBokCableComponent::GetAttachedComponent() const
{
	return AttachEndTo.GetComponent(GetOwner());
}

void UBokCableComponent::GetCableParticleLocations(TArray<FVector>& Locations) const
{
	Locations.Empty();
	for (const FBokCableParticle& Particle : Particles)
	{
		Locations.Add(Particle.Position);
	}
}


void UBokCableComponent::GetEndPositions(FVector& OutStartPosition, FVector& OutEndPosition)
{
	// Start position is just component position
	OutStartPosition = GetComponentLocation();

	// See if we want to attach the other end to some other component
	USceneComponent* EndComponent = AttachEndTo.GetComponent(GetOwner());
	if(EndComponent == NULL)
	{
		EndComponent = this;
	}

	if (AttachEndToSocketName != NAME_None)
	{
		OutEndPosition = EndComponent->GetSocketTransform(AttachEndToSocketName).TransformPosition(EndLocation);
	}
	else
	{
		OutEndPosition = EndComponent->GetComponentTransform().TransformPosition(EndLocation);
	}

}

void UBokCableComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FVector Gravity = FVector(0, 0, GetWorld()->GetGravityZ()) * CableGravityScale;

	// Update end points
	FVector CableStart, CableEnd;
	GetEndPositions(CableStart, CableEnd);

	FBokCableParticle& StartParticle = Particles[0];

	if (bAttachStart)
	{
		StartParticle.Position = StartParticle.OldPosition = CableStart;
		StartParticle.bFree = false;
	}
	else
	{
		StartParticle.bFree = true;
	}

	FBokCableParticle& EndParticle = Particles[NumSegments];
	if (bAttachEnd)
	{
		EndParticle.Position = EndParticle.OldPosition = CableEnd;
		EndParticle.bFree = false;
	}
	else
	{
		EndParticle.bFree = true;
	}

	// Ensure a non-zero substep
	float UseSubstep = FMath::Max(SubstepTime, 0.005f);

	// Perform simulation substeps
	TimeRemainder += DeltaTime;
	while(TimeRemainder > UseSubstep)
	{
		PerformSubstep(UseSubstep, Gravity);
		TimeRemainder -= UseSubstep;
	}

	// Need to send new data to render thread
	MarkRenderDynamicDataDirty();

	// Call this because bounds have changed
	UpdateComponentToWorld();
};

void UBokCableComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	SendRenderDynamicData_Concurrent();
}

void UBokCableComponent::ApplyWorldOffset(const FVector & InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);

	for (FBokCableParticle& Particle : Particles)
	{
		Particle.Position += InOffset;
		Particle.OldPosition += InOffset;
	}
}

void UBokCableComponent::SendRenderDynamicData_Concurrent()
{
	if(SceneProxy)
	{
		// Allocate cable dynamic data
		FBokCableDynamicData* DynamicData = new FBokCableDynamicData;

		// Transform current positions from particles into component-space array
		const FTransform& ComponentTransform = GetComponentTransform();
		int32 NumPoints = NumSegments+1;
		DynamicData->CablePoints.AddUninitialized(NumPoints);
		for(int32 PointIdx=0; PointIdx<NumPoints; PointIdx++)
		{
			DynamicData->CablePoints[PointIdx] = ComponentTransform.InverseTransformPosition(Particles[PointIdx].Position);
		}

		// Enqueue command to send to render thread
		FBokCableSceneProxy* CableSceneProxy = (FBokCableSceneProxy*)SceneProxy;
		ENQUEUE_RENDER_COMMAND(FSendCableDynamicData)(
			[CableSceneProxy, DynamicData](FRHICommandListImmediate& RHICmdList)
		{
			CableSceneProxy->SetDynamicData_RenderThread(DynamicData);
		});
	}
}

FBoxSphereBounds UBokCableComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// Calculate bounding box of cable points
	FBox CableBox(ForceInit);
	for(int32 ParticleIdx=0; ParticleIdx<Particles.Num(); ParticleIdx++)
	{
		const FBokCableParticle& Particle = Particles[ParticleIdx];
		CableBox += Particle.Position;
	}

	// Expand by cable radius (half cable width)
	return FBoxSphereBounds(CableBox.ExpandBy(0.5f * CableWidth));
}

void UBokCableComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const 
{
	OutSockets.Add(FComponentSocketDescription(CableEndSocketName, EComponentSocketType::Socket));
	OutSockets.Add(FComponentSocketDescription(CableStartSocketName, EComponentSocketType::Socket));
}

FTransform UBokCableComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	int32 NumParticles = Particles.Num();
	if ((InSocketName == CableEndSocketName || InSocketName == CableStartSocketName) && NumParticles >= 2)
	{
		FVector ForwardDir, Pos;
		if (InSocketName == CableEndSocketName)
		{
			FVector LastPos = Particles[NumParticles - 1].Position;
			FVector PreviousPos = Particles[NumParticles - 2].Position;

			ForwardDir = (LastPos - PreviousPos).GetSafeNormal();
			Pos = LastPos;
		}
		else
		{
			FVector FirstPos = Particles[0].Position;
			FVector NextPos = Particles[1].Position;

			ForwardDir = (NextPos - FirstPos).GetSafeNormal();
			Pos = FirstPos;
		}

		const FQuat RotQuat = FQuat::FindBetween(FVector(1, 0, 0), ForwardDir);
		FTransform WorldSocketTM = FTransform(RotQuat, Pos, FVector(1, 1, 1));

		switch (TransformSpace)
		{
			case RTS_World:
			{
				return WorldSocketTM;
			}
			case RTS_Actor:
			{
				if (const AActor* Actor = GetOwner())
				{
					return WorldSocketTM.GetRelativeTransform(GetOwner()->GetTransform());
				}
				break;
			}
			case RTS_Component:
			{
				return WorldSocketTM.GetRelativeTransform(GetComponentTransform());
			}
		}
	}

	return Super::GetSocketTransform(InSocketName, TransformSpace);
}

bool UBokCableComponent::HasAnySockets() const
{
	return (Particles.Num() >= 2);
}

bool UBokCableComponent::DoesSocketExist(FName InSocketName) const
{
	return (InSocketName == CableEndSocketName) || (InSocketName == CableStartSocketName);
}
