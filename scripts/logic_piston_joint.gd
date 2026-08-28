class_name LogicPistonJoint
extends Generic6DOFJoint3D

var signal_enabled := false
var travel := 3.2
var speed := 1.35
var force_limit := 520.0

func _ready() -> void:
    add_to_group("logic_receivers")
    _configure_constraints()
    _apply_motor()

func configure(p_travel: float = 3.2, p_speed: float = 1.35, p_force_limit: float = 520.0) -> void:
    travel = maxf(0.25, p_travel)
    speed = maxf(0.1, p_speed)
    force_limit = maxf(1.0, p_force_limit)

func _configure_constraints() -> void:
    # Local X is the only translational degree of freedom.
    set_flag_x(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
    set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, -travel * 0.5)
    set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, travel * 0.5)
    set_flag_x(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_MOTOR, true)
    set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_MOTOR_FORCE_LIMIT, force_limit)

    set_flag_y(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
    set_param_y(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, 0.0)
    set_param_y(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, 0.0)
    set_flag_z(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
    set_param_z(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, 0.0)
    set_param_z(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, 0.0)

    # Lock every angular axis so the connected body behaves like a linear actuator.
    set_flag_x(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
    set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
    set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)
    set_flag_y(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
    set_param_y(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
    set_param_y(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)
    set_flag_z(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
    set_param_z(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
    set_param_z(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)

func _apply_motor() -> void:
    var velocity := speed if signal_enabled else -speed
    set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_MOTOR_TARGET_VELOCITY, velocity)

func set_signal(value: bool) -> void:
    signal_enabled = value
    _apply_motor()

func get_prompt_text() -> String:
    return "[E] Piston  %s  |  logic input extends/retracts" % ("EXTEND" if signal_enabled else "RETRACT")

func interact(_player: Node) -> void:
    set_signal(not signal_enabled)
