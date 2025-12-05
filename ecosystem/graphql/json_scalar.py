import strawberry
from typing import Any

@strawberry.scalar(
    name="JSONScalar",
    description="Custom JSON scalar"
)
def JSONScalar(value: Any) -> Any:
    return value
