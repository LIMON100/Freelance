#config.py
from pydantic import BaseModel
from typing import Optional

class Configuration(BaseModel):
    user_id: str
    store: Optional[object] = None
    class Config:
      arbitrary_types_allowed = True

    @classmethod
    def from_runnable_config(cls, config):
      return cls(**config.get("configurable",{}))