// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "LearnedMMCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ALearnedMMCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	ALearnedMMCharacter();


protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
public:

	//SkeletalMesh대신 관절을 움직일수 있는 UPoseableMeshComponent 사용 할 예정.
	UPROPERTY(Category = Character, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPoseableMeshComponent> PoseableMesh;

	//UCharacter 클래스에 존재하는 GetMesh() 함수 재정의 -> PosealbeMeshComponent를 가져옴.
	UFUNCTION()
	virtual UPoseableMeshComponent* GetMesh() const;

private:
	UPROPERTY()
	FRotator CharacterCurrentRotation = FRotator(0,0,0);
	UPROPERTY()
	FRotator CharacterGaolRotation;

	UPROPERTY()
	FVector2D LeftStickValue;
	UPROPERTY()
	FVector2D RightStickValue;

	UPROPERTY()
	bool IsHandlingRightStick = false;

	//UPROPERTY()
	//class APlayerController* PlayerController;

// 오렌지덕씨 함수
public:
	UFUNCTION()
	void desired_gait_update(float& desired_gait, float& desired_gait_velocity, const float dt, const float gait_change_halflife);
};

