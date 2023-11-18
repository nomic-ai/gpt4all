import requests
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field
from typing import List, Dict

# Define the router for the engines module
router = APIRouter(prefix="/engines", tags=["Search Endpoints"])

# Define the models for the engines module
class ListEnginesResponse(BaseModel):
    data: List[Dict] = Field(..., description="All available models.")

class EngineResponse(BaseModel):
    data: List[Dict] = Field(..., description="All available models.")


# Define the routes for the engines module
@router.get("/", response_model=ListEnginesResponse)
async def list_engines():
    try:
        response = requests.get('https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models2.json')
        response.raise_for_status()  # This will raise an HTTPError if the HTTP request returned an unsuccessful status code
        engines = response.json()
        return ListEnginesResponse(data=engines)
    except requests.RequestException as e:
        logger.error(f"Error fetching engine list: {e}")
        raise HTTPException(status_code=500, detail="Error fetching engine list")

# Define the routes for the engines module
@router.get("/{engine_id}", response_model=EngineResponse)
async def retrieve_engine(engine_id: str):
    try:
        # Implement logic to fetch a specific engine's details
        # This is a placeholder, replace with your actual data retrieval logic
        engine_details = {"id": engine_id, "name": "Engine Name", "description": "Engine Description"}
        return EngineResponse(data=[engine_details])
    except Exception as e:
        logger.error(f"Error fetching engine details: {e}")
        raise HTTPException(status_code=500, detail=f"Error fetching details for engine {engine_id}")