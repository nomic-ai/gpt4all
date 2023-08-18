import logging
from fastapi import APIRouter
from fastapi.responses import JSONResponse

log = logging.getLogger(__name__)

router = APIRouter(prefix="/health", tags=["Health"])


@router.get('/', response_class=JSONResponse)
async def health_check():
    """Runs a health check on this instance of the API."""
    return JSONResponse({'status': 'ok'}, headers={'Access-Control-Allow-Origin': '*'})
