extends Node


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	print("Pre-connection check...")
	if QuicNetworkSystem:
		print("QNS System detected!")
		QuicNetworkSystem.connect_to_server("192.168.0.207", 7777)
	else:
		print("CRITICAL: QNS System NOT detected. Check your GDExtension logs.")


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
