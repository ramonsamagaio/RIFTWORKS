class_name LogicMotorJoint
extends HingeJoint3D

var signal_enabled := true
var target_velocity := 8.5
var max_impulse := 28.0

func _ready() -> void:
    add_to_group("logic_receivers")
    set_param(HingeJoint3D.PARAM_MOTOR_TARGET_VELOCITY, target_velocity)
    set_param(HingeJoint3D.PARAM_MOTOR_MAX_IMPULSE, max_impulse)
    set_signal(signal_enabled)

func set_signal(value: bool) -> void:
    signal_enabled = value
    set_flag(HingeJoint3D.FLAG_ENABLE_MOTOR, value)

func get_prompt_text() -> String:
    return "Logic Motor  <- %s" % ("HIGH" if signal_enabled else "LOW")
