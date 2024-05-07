// Copyright Epic Games, Inc. All Rights Reserved.
#include "LearnedMMCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ALearnedMMCharacter

ALearnedMMCharacter::ALearnedMMCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}


void ALearnedMMCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Tick �Լ� ���
	PrimaryActorTick.bCanEverTick = true;

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	DampingPosition = GetMesh()->GetComponentLocation()+FVector(0, 0, 10);;
}

void ALearnedMMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	FVector Start = GetMesh()->GetComponentLocation();
	FVector End = Start + GetActorForwardVector() * 300;

	MoveGoalPosition = Start + FVector(0, 0, 10);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red);

	DrawDebugDirectionalArrow(GetWorld(), Start, End, 500, FColor::Red);

	//DampingPosition = Lerp(DampingPosition, MoveGoalPosition, 0.01);
	//DampingPosition = damper_exponential(DampingPosition, MoveGoalPosition, 1, DeltaTime);
	DampingPosition = damper_exact(DampingPosition, MoveGoalPosition, 0.9, DeltaTime);
	DrawDebugSphere(GetWorld(), DampingPosition, 10, 16, FColor::Blue);

	//FVector MoveValue = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetInputVectorAxisValue());
	//UE_LOG(LogTemp, Log, TEXT("%f   %f"), MoveValue.X, MoveValue.Y);
}


//�������� �޶����� �ӵ��� �޶����� ������ ����. 
// -> DeltaTime�� ���Ͽ� �����ذᰡ��. ������ �����̳� dt�� �ʹ� ���� �����ϸ� (��, damping * dt > 1�� �Ǵ� ���) ��ü �ý����� �Ҿ���������, �־��� ��� ������ �� �ֽ��ϴ�
// -> damper_exponential ������� �ذᰡ��.
FVector ALearnedMMCharacter::Lerp(FVector Pos, FVector GoalPos, float factor)
{	
	return (1.0 - factor) * Pos + factor * GoalPos;
}


// ������ ������ Ư���� Ÿ�� ���ܿ� ���ߵ� ���� �ӵ��� ������ ���� �����ϰ� ��������ν� ������ �ذ�
FVector ALearnedMMCharacter::damper_exponential(FVector x, FVector g, float damping, float dt)
{
	float ft = 1.0f / 60.0f;
	return Lerp(x, g, 1.0f - powf(1.0 / (1.0 - ft * damping), -dt / ft));
}

FVector ALearnedMMCharacter::damper_exact(FVector x, FVector g, float halflife, float dt)
{
	float eps = 1e-5f;
	return Lerp(x, g, 1.0f - fast_negexp((0.69314718056f * dt) / (halflife + eps)));
}

//���� ���� �Լ��� ������ ���׽��� ������ ������ �ٻ��ϴ� ���Դϴ�
//�Ҿ����� ����⸦ ������ �������̸� �������� �Ű������� ���� ������ ��ȯ
float ALearnedMMCharacter::fast_negexp(float x)
{
	return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ALearnedMMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALearnedMMCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALearnedMMCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ALearnedMMCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	UE_LOG(LogTemp, Log, TEXT("%f   %f"), MovementVector.X, MovementVector.Y);

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);


		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);


	}
}

void ALearnedMMCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}