"""
Ollama API Client
=================

Handles communication with the Ollama API for LLM inference.
"""

import requests
import json
from typing import Optional, Dict, Any, List, Generator
from dataclasses import dataclass


@dataclass
class OllamaConfig:
    """Configuration for Ollama client."""
    base_url: str = "http://localhost:11434"
    model: str = "qwen2.5:7b-instruct-q4_K_M"
    temperature: float = 0.7
    num_ctx: int = 4096
    num_predict: int = 2048
    top_p: float = 0.9
    repeat_penalty: float = 1.1
    stop_sequences: List[str] = None
    timeout: int = 120
    
    def __post_init__(self):
        if self.stop_sequences is None:
            self.stop_sequences = ["---", "## YOUR TASK", "Note:", "I hope", "Let me"]


class OllamaClient:
    """Client for interacting with Ollama API."""
    
    def __init__(self, config: Optional[OllamaConfig] = None):
        """
        Initialize the Ollama client.
        
        Args:
            config: Optional configuration. Uses defaults if not provided.
        """
        self.config = config or OllamaConfig()
    
    def is_available(self) -> bool:
        """Check if Ollama server is running and accessible."""
        try:
            response = requests.get(
                f"{self.config.base_url}/api/tags",
                timeout=5
            )
            return response.status_code == 200
        except requests.exceptions.RequestException:
            return False
    
    def list_models(self) -> List[str]:
        """Get list of available models."""
        try:
            response = requests.get(
                f"{self.config.base_url}/api/tags",
                timeout=5
            )
            if response.status_code == 200:
                models = response.json().get("models", [])
                return [m["name"] for m in models]
        except requests.exceptions.RequestException:
            pass
        return []
    
    def has_model(self, model_name: Optional[str] = None) -> bool:
        """Check if a specific model is available."""
        model = model_name or self.config.model
        models = self.list_models()
        # Check for exact match or base name match
        base_name = model.split(":")[0]
        return any(model == m or base_name in m for m in models)
    
    def generate(
        self,
        prompt: str,
        stream: bool = False,
        **kwargs
    ) -> str:
        """
        Generate completion from the LLM.
        
        Args:
            prompt: The prompt to send to the model
            stream: Whether to stream the response
            **kwargs: Override any config options
            
        Returns:
            Generated text response
            
        Raises:
            RuntimeError: If the API call fails
        """
        payload = {
            "model": kwargs.get("model", self.config.model),
            "prompt": prompt,
            "stream": stream,
            "options": {
                "temperature": kwargs.get("temperature", self.config.temperature),
                "num_ctx": kwargs.get("num_ctx", self.config.num_ctx),
                "num_predict": kwargs.get("num_predict", self.config.num_predict),
                "top_p": kwargs.get("top_p", self.config.top_p),
                "repeat_penalty": kwargs.get("repeat_penalty", self.config.repeat_penalty),
                "stop": kwargs.get("stop", self.config.stop_sequences),
            }
        }
        
        try:
            response = requests.post(
                f"{self.config.base_url}/api/generate",
                json=payload,
                timeout=self.config.timeout
            )
            
            if response.status_code == 200:
                return response.json().get("response", "")
            else:
                raise RuntimeError(
                    f"Ollama API error: {response.status_code} - {response.text}"
                )
                
        except requests.exceptions.Timeout:
            raise RuntimeError("Ollama request timed out")
        except requests.exceptions.ConnectionError:
            raise RuntimeError(
                "Cannot connect to Ollama. Is it running? Start with: ollama serve"
            )
    
    def generate_stream(
        self,
        prompt: str,
        **kwargs
    ) -> Generator[str, None, None]:
        """
        Generate completion with streaming.
        
        Args:
            prompt: The prompt to send to the model
            **kwargs: Override any config options
            
        Yields:
            Chunks of generated text
        """
        payload = {
            "model": kwargs.get("model", self.config.model),
            "prompt": prompt,
            "stream": True,
            "options": {
                "temperature": kwargs.get("temperature", self.config.temperature),
                "num_ctx": kwargs.get("num_ctx", self.config.num_ctx),
                "num_predict": kwargs.get("num_predict", self.config.num_predict),
                "top_p": kwargs.get("top_p", self.config.top_p),
                "repeat_penalty": kwargs.get("repeat_penalty", self.config.repeat_penalty),
                "stop": kwargs.get("stop", self.config.stop_sequences),
            }
        }
        
        try:
            with requests.post(
                f"{self.config.base_url}/api/generate",
                json=payload,
                stream=True,
                timeout=self.config.timeout
            ) as response:
                if response.status_code != 200:
                    raise RuntimeError(
                        f"Ollama API error: {response.status_code}"
                    )
                
                for line in response.iter_lines():
                    if line:
                        data = json.loads(line)
                        if "response" in data:
                            yield data["response"]
                        if data.get("done", False):
                            break
                            
        except requests.exceptions.Timeout:
            raise RuntimeError("Ollama request timed out")
        except requests.exceptions.ConnectionError:
            raise RuntimeError("Cannot connect to Ollama")
