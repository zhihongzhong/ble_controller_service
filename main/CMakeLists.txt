set(CompileList 
    storage.c
    main.c
    motor.c
    bluetooth_controller.c
    task_temperature.c
    task_hibernate.c
    instruction.c
    temperature.c
    speed_calculator.c
) 

idf_component_register(SRCS "${CompileList}"
                    INCLUDE_DIRS "")
