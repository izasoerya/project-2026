from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, ConfigDict
from datetime import datetime

app = FastAPI()


# ============================================================
# FLOOR IDs
# ============================================================

FLOOR_1 = "433b141d-b7db-415f-86e0-c42f322dbeff"
FLOOR_2 = "433b141d-b7db-415f-86e0-c42f322dbefg"
FLOOR_3 = "433b141d-b7db-415f-86e0-c42f322dbefh"

VALID_FLOORS = {
    FLOOR_1,
    FLOOR_2,
    FLOOR_3,
}


# ============================================================
# RESPONSE MODELS
# ============================================================

class SensorResponse(BaseModel):
    id: int
    topic: str
    floor_entity_id: str
    operational_entity_id: int
    temperature: float
    humidity: float
    ammonia: float
    wind_speed: float
    light_intensity: float
    rssi: float
    counter: int
    free_heap: int
    largest_free_block: int
    min_free_heap: int
    last_reset_reason: str
    created_at: datetime

    model_config = ConfigDict(
        from_attributes=True,
    )


class ActuatorModeResponse(BaseModel):
    id: int
    floor_entity_id: str
    duration_intermittent_blower: int
    duration_intermittent_pump: int
    top_temperature_target_pump: float
    top_temperature_target_heater: float
    bottom_temperature_target_pump: float
    bottom_temperature_target_heater: float
    top_temperature_target_blower: float
    bottom_temperature_target_blower: float
    created_at: datetime

    model_config = ConfigDict(
        from_attributes=True,
    )


class ActuatorResponse(BaseModel):
    id: int
    topic: str
    floor_entity_id: str
    blower_value: list[float]
    pump_value: list[bool]
    heater_value: list[bool]
    lamp_value: list[int]
    mode_blower: str
    mode_pump: str
    mode_heater: str
    mode_lamp: str
    created_at: datetime

    model_config = ConfigDict(
        from_attributes=True,
    )


# ============================================================
# MOCK DATA
# ============================================================

sensor_data = {

    # --------------------------------------------------------
    # FLOOR 1
    # --------------------------------------------------------
    FLOOR_1: SensorResponse(
        id=1,
        topic="sensor/floor-1",
        floor_entity_id=FLOOR_1,
        operational_entity_id=1,
        temperature=22.5,
        humidity=65.0,
        ammonia=0.5,
        wind_speed=1.2,
        light_intensity=500.0,
        rssi=-65.0,
        counter=42,
        free_heap=128000,
        largest_free_block=64000,
        min_free_heap=96000,
        last_reset_reason="power_on",
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 2
    # --------------------------------------------------------
    FLOOR_2: SensorResponse(
        id=2,
        topic="sensor/floor-2",
        floor_entity_id=FLOOR_2,
        operational_entity_id=2,
        temperature=27.8,
        humidity=72.0,
        ammonia=1.2,
        wind_speed=2.8,
        light_intensity=750.0,
        rssi=-58.0,
        counter=108,
        free_heap=115000,
        largest_free_block=58000,
        min_free_heap=85000,
        last_reset_reason="software_reset",
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 3
    # --------------------------------------------------------
    FLOOR_3: SensorResponse(
        id=3,
        topic="sensor/floor-3",
        floor_entity_id=FLOOR_3,
        operational_entity_id=3,
        temperature=30.2,
        humidity=80.0,
        ammonia=2.1,
        wind_speed=3.6,
        light_intensity=900.0,
        rssi=-72.0,
        counter=256,
        free_heap=102000,
        largest_free_block=47000,
        min_free_heap=70000,
        last_reset_reason="brownout",
        created_at=datetime.now(),
    ),
}


actuator_mode_data = {

    # --------------------------------------------------------
    # FLOOR 1
    # --------------------------------------------------------
    FLOOR_1: ActuatorModeResponse(
        id=1,
        floor_entity_id=FLOOR_1,
        duration_intermittent_blower=30,
        duration_intermittent_pump=45,
        top_temperature_target_pump=26.0,
        top_temperature_target_heater=28.0,
        bottom_temperature_target_pump=25.0,
        bottom_temperature_target_heater=27.0,
        top_temperature_target_blower=24.0,
        bottom_temperature_target_blower=23.0,
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 2
    # --------------------------------------------------------
    FLOOR_2: ActuatorModeResponse(
        id=2,
        floor_entity_id=FLOOR_2,
        duration_intermittent_blower=40,
        duration_intermittent_pump=60,
        top_temperature_target_pump=28.0,
        top_temperature_target_heater=30.0,
        bottom_temperature_target_pump=27.0,
        bottom_temperature_target_heater=29.0,
        top_temperature_target_blower=26.0,
        bottom_temperature_target_blower=25.0,
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 3
    # --------------------------------------------------------
    FLOOR_3: ActuatorModeResponse(
        id=3,
        floor_entity_id=FLOOR_3,
        duration_intermittent_blower=50,
        duration_intermittent_pump=75,
        top_temperature_target_pump=30.0,
        top_temperature_target_heater=32.0,
        bottom_temperature_target_pump=29.0,
        bottom_temperature_target_heater=31.0,
        top_temperature_target_blower=28.0,
        bottom_temperature_target_blower=27.0,
        created_at=datetime.now(),
    ),
}


actuator_data = {

    # --------------------------------------------------------
    # FLOOR 1
    # --------------------------------------------------------
    FLOOR_1: ActuatorResponse(
        id=1,
        topic="actuator/floor-1",
        floor_entity_id=FLOOR_1,
        blower_value=[50.0, 75.0, 100.0],
        pump_value=[True, False, True],
        heater_value=[False, True, True, False],
        lamp_value=[255, 128, 0],
        mode_blower="auto",
        mode_pump="intermittent",
        mode_heater="manual",
        mode_lamp="auto",
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 2
    # --------------------------------------------------------
    FLOOR_2: ActuatorResponse(
        id=2,
        topic="actuator/floor-2",
        floor_entity_id=FLOOR_2,
        blower_value=[25.0, 50.0, 80.0],
        pump_value=[False, True, True],
        heater_value=[False, True, False, True],
        lamp_value=[200, 180, 100],
        mode_blower="manual",
        mode_pump="auto",
        mode_heater="auto",
        mode_lamp="manual",
        created_at=datetime.now(),
    ),

    # --------------------------------------------------------
    # FLOOR 3
    # --------------------------------------------------------
    FLOOR_3: ActuatorResponse(
        id=3,
        topic="actuator/floor-3",
        floor_entity_id=FLOOR_3,
        blower_value=[80.0, 90.0, 100.0],
        pump_value=[True, True, True],
        heater_value=[False, False, True, False],
        lamp_value=[255, 255, 150],
        mode_blower="auto",
        mode_pump="auto",
        mode_heater="auto",
        mode_lamp="auto",
        created_at=datetime.now(),
    ),
}


# ============================================================
# VALIDATE FLOOR
# ============================================================

def validate_floor(floor_entity_id: str):
    if floor_entity_id not in VALID_FLOORS:
        raise HTTPException(
            status_code=404,
            detail=f"Floor entity not found: {floor_entity_id}",
        )


# ============================================================
# SENSOR
# ============================================================

@app.get(
    "/sensor/find-latest/{floor_entity_id}",
    response_model=SensorResponse,
)
async def get_sensor(floor_entity_id: str):

    validate_floor(floor_entity_id)

    return sensor_data[floor_entity_id]


# ============================================================
# ACTUATOR MODE
# ============================================================

@app.get(
    "/actuator-mode/find-latest/{floor_entity_id}",
    response_model=ActuatorModeResponse,
)
async def get_actuator_mode(floor_entity_id: str):

    validate_floor(floor_entity_id)

    return actuator_mode_data[floor_entity_id]


# ============================================================
# ACTUATOR
# ============================================================

@app.get(
    "/actuator/find-latest/{floor_entity_id}",
    response_model=ActuatorResponse,
)
async def get_actuator(floor_entity_id: str):

    validate_floor(floor_entity_id)

    return actuator_data[floor_entity_id]


# ============================================================
# HEALTH
# ============================================================

@app.get("/health")
async def health_check():

    return {
        "status": "ok",
        "timestamp": datetime.now().isoformat(),
    }


# ============================================================
# RUN
# ============================================================

if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
    )
