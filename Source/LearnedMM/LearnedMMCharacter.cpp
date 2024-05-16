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
#include "Components/PoseableMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "MMCommon.h"
#include "MMArray.h"
#include "MMQuat.h"
#include "MMVec.h"
#include "MMSpring.h"



//--------------------------------------------------------------------------//
//					 회전변환 함수											
FVector2D RotationTransForm2D(FVector2D V, float a)
{
	FVector2D OutVector;
	float cosA = FMath::Cos(a);
	float sinA = FMath::Sin(a);

	OutVector.X = cosA * V.X - sinA * V.Y;
	OutVector.Y = sinA * V.X + cosA * V.Y;

	return OutVector;
}

FVector RotationTransForm3D_Z(FVector V, float a)
{
	FVector OutVector;
	float cosA = FMath::Cos(a);
	float sinA = FMath::Sin(a);

	OutVector.X = cosA * V.X - sinA * V.Y;
	OutVector.Y = sinA * V.X + cosA * V.Y;
	OutVector.Z = V.Z;

	return OutVector;
}

//FRotator.Yaw값을 정규화된 Vector로표현
FVector RotatorToVector(FRotator Rotator)
{
	float YawDegrees = Rotator.Yaw;
	float YawRadians = FMath::DegreesToRadians(YawDegrees);

	FVector NormalizedVector = FVector(FMath::Cos(YawRadians), FMath::Sin(YawRadians), 0.0f);

	return NormalizedVector;
}

//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//
//						조이스틱 변환 관련 함수								

float CalculateJoystickAngle(FVector2D JoyStick)
{
	float JoystickAngle = FMath::RadiansToDegrees(FMath::Atan2(JoyStick.X, JoyStick.Y));


	return JoystickAngle;
}

FRotator CalculateJoystickAngle_FRotator(FVector2D JoyStick)
{
	float JoystickAngle = FMath::RadiansToDegrees(FMath::Atan2(JoyStick.X, JoyStick.Y));

	return FRotator(0, JoystickAngle, 0);
}

FQuat CalculateJoystickAngle_FQuat(FVector2D JoyStick)
{
	float JoystickAngle = FMath::RadiansToDegrees(FMath::Atan2(JoyStick.X, JoyStick.Y));

	return FRotator(0, JoystickAngle, 0).Quaternion();
}

////////////////////////////////////////////////////////////////////////
// 오렌지덕씨 함수 시작
//--------------------------------------------------------------------------//
void ALearnedMMCharacter::OnStrafe(const FInputActionValue& Value)
{
	bStrafe = true;
}

void ALearnedMMCharacter::StopStrafe(const FInputActionValue& Value)
{
	bStrafe = false;	
}

void ALearnedMMCharacter::desired_gait_update(float& desired_gait, float& desired_gait_velocity, const float dt, const float gait_change_halflife = 0.1f)
{	
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController == nullptr) return;
	
	simple_spring_damper_exact(
		desired_gait,
		desired_gait_velocity,
		PlayerController->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom) ? 1.0f : 0.0f,
		gait_change_halflife,
		dt);
}


vec3 desired_velocity_update(
	const vec3 gamepadstick_left,
	const float camera_azimuth,
	const quat simulation_rotation,
	const float fwrd_speed,
	const float side_speed,
	const float back_speed)
{
	// Find stick position in world space by rotating using camera azimuth
	vec3 global_stick_direction = quat_mul_vec3(
		quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), gamepadstick_left);

	// Find stick position local to current facing direction
	vec3 local_stick_direction = quat_inv_mul_vec3(
		simulation_rotation, global_stick_direction);

	// Scale stick by forward, sideways and backwards speeds
	vec3 local_desired_velocity = local_stick_direction.z > 0.0 ? vec3(side_speed, 0.0f, fwrd_speed) * local_stick_direction : vec3(side_speed, 0.0f, back_speed) * local_stick_direction;

	// Re-orientate into the world space
	return quat_mul_vec3(simulation_rotation, local_desired_velocity);
}


quat desired_rotation_update(
	const quat desired_rotation,
	const vec3 gamepadstick_left,
	const vec3 gamepadstick_right,
	const float camera_azimuth,
	const bool desired_strafe,
	const vec3 desired_velocity)
{
	quat desired_rotation_curr = desired_rotation;

	// If strafe is active then desired direction is coming from right
	// stick as long as that stick is being used, otherwise we assume
	// forward facing

	// strafe가 활성화되어 있으면 원하는 방향은 오른쪽 스틱에서 오는 것이고,
	// 해당 스틱이 사용 중인 경우에는 전방 향을 가정합니다.
	if (desired_strafe)
	{
		vec3 desired_direction = quat_mul_vec3(quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), vec3(0, 0, -1));

		if (length(gamepadstick_right) > 0.01f)
		{
			desired_direction = quat_mul_vec3(quat_from_angle_axis(camera_azimuth, vec3(0, 1, 0)), normalize(gamepadstick_right));
		}

		return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), vec3(0, 1, 0));
	}

	// If strafe is not active the desired direction comes from the left 
	// stick as long as that stick is being used

	// strafe가 비활성화되어 있으면 원하는 방향은 해당 스틱이 사용 중인 동안 왼쪽 스틱에서 나옵니다.
	else if (length(gamepadstick_left) > 0.01f)
	{
		vec3 desired_direction = normalize(desired_velocity);
		return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), vec3(0, 1, 0));
	}

	// Otherwise desired direction remains the same
	else
	{
		return desired_rotation_curr;
	}
}


DEFINE_LOG_CATEGORY(LogTemplateCharacter);
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


	//PoseableMesh를 생성하고 RootComponent에 종속 시킴.
	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMesh"));
	PoseableMesh->SetupAttachment(RootComponent);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}


void ALearnedMMCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Tick 함수 등록
	PrimaryActorTick.bCanEverTick = true;

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	//PlayerController = UGameplayStatics::GetPlayerController(this, 0);
}

void ALearnedMMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//좌측 조이스틱(이동) 키를 입력하지 않으면 입력값 초기화.
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftStick_Right))
		{
			LeftStickValue = FVector2D(0, 0);
		}
	}

	//우측 조이스틱(회전) 키를 입력하는지 체크
	if (PlayerController)
	{
		if (!PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Down) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Up)
			&& !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Left) && !PlayerController->IsInputKeyDown(EKeys::Gamepad_RightStick_Right))
		{
			IsHandlingRightStick = false;
		}
		else
		{
			IsHandlingRightStick = true;
		}
	}



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

		// Strafe
		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Started, this, &ALearnedMMCharacter::OnStrafe);
		EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &ALearnedMMCharacter::StopStrafe);
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
	float DeltaTime = GetWorld()->GetDeltaSeconds();

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

	///// LeftStick Value를 캐릭터 Look Axis 기준으로 회전 변환.
	//if (Controller != nullptr)
	//{
	//	// find out which way is forward
	//	const FRotator Rotation = Controller->GetControlRotation();
	//	float Radian = -FMath::DegreesToRadians(Rotation.Yaw);

	//	LeftStickValue = RotationTransForm2D(MovementVector, Radian);
	//}


	///// 만약 좌측 조이스틱 입력이 없으면 Move방향으로 캐릭터 회전 및 보간.
	//if (IsHandlingRightStick == false)
	//{
	//	FRotator LeftJoystickAngle = CalculateJoystickAngle_FRotator(MovementVector);
	//	FRotator CalculatedRotation = FMath::RInterpTo(GetActorRotation(), LeftJoystickAngle, DeltaTime, 2.0f);

	//	SetActorRotation(CalculatedRotation);

	//	CharacterGaolRotation = CalculateJoystickAngle_FRotator(MovementVector);
	//	CharacterCurrentRotation = FMath::RInterpTo(CharacterCurrentRotation, CharacterGaolRotation, DeltaTime, 2.0f);
	//	SetActorRotation(CharacterGaolRotation);
	//}
}

void ALearnedMMCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	
	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}


	//-----------------------------------------------------------------------------//
	//CharacterGaolRotation = CalculateJoystickAngle_FRotator(LookAxisVector);
	//CharacterCurrentRotation = FMath::RInterpTo(CharacterCurrentRotation, CharacterGaolRotation, DeltaTime, 2.0f);
	//SetActorRotation(CharacterGaolRotation);


	//-----------------------------------------------------------------------------//
	//PoseableMesh를 활용해 관절 움직임 테스트 코드
	/*FRotator R = GetMesh()->GetBoneRotationByName(TEXT("neck_01"), EBoneSpaces::WorldSpace);
	//GetMesh()->SetBoneRotationByName(TEXT("neck_01"), R + FRotator(0, 10, 0), EBoneSpaces::WorldSpace);*/
	//-----------------------------------------------------------------------------//

}

//UCharacter 클래스에 존재하는 GetMesh() 함수 재정의 -> PosealbeMeshComponent를 가져옴.
UPoseableMeshComponent* ALearnedMMCharacter::GetMesh() const
{
	return PoseableMesh;
}