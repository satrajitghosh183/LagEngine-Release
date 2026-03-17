"""TensorBoard and console logging for RL training."""

import os
from pathlib import Path
from typing import Dict, Optional
import numpy as np


class _NoOpWriter:
    """Dummy writer when tensorboard is not installed."""

    def add_scalar(self, key: str, value: float, step: int):
        pass

    def close(self):
        pass


class Logger:
    """TensorBoard + console logger."""

    def __init__(self, logdir: str, exp_name: str = "rl"):
        self.logdir = Path(logdir)
        self.logdir.mkdir(parents=True, exist_ok=True)
        self.exp_name = exp_name
        self._writer = None
        self._step = 0

    @property
    def writer(self):
        if self._writer is None:
            try:
                from torch.utils.tensorboard import SummaryWriter
                self._writer = SummaryWriter(log_dir=str(self.logdir))
            except ImportError:
                self._writer = _NoOpWriter()
        return self._writer

    def log_scalar(self, key: str, value: float, step: Optional[int] = None):
        step = step if step is not None else self._step
        if isinstance(value, (int, float)):
            self.writer.add_scalar(key, float(value), step)

    def log_scalars(self, metrics: Dict[str, float], step: Optional[int] = None):
        step = step if step is not None else self._step
        for k, v in metrics.items():
            self.writer.add_scalar(k, v, step)

    def set_step(self, step: int):
        self._step = step

    def close(self):
        if self._writer is not None:
            self._writer.close()
            self._writer = None
