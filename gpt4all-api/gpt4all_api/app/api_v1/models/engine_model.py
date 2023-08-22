from typing import List, Optional
from pydantic import BaseModel, Field


class Engine(BaseModel):
    order: str
    md5sum: str
    name: str
    filename: str
    filesize: str
    requires: Optional[str]
    ramrequired: str
    parameters: str
    quant: str
    type: str
    description: str
    url: Optional[str]
    promptTemplate: Optional[str]
    systemPrompt: str

# responses
class ListEnginesResponse(BaseModel):
    data: List[Engine] = Field(..., description="All available models.")


class EngineResponse(BaseModel):
    data: List[Engine] = Field(..., description="All available models.")