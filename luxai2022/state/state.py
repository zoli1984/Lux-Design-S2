from dataclasses import dataclass, field
from typing import Dict, List

import numpy as np
from luxai2022.config import EnvConfig
from luxai2022.factory import Factory
from luxai2022.map.board import Board
from luxai2022.team import Team

from luxai2022.unit import Unit
from collections import OrderedDict
import copy
import numba

@dataclass
class State:
    seed_rng: np.random.RandomState
    seed: int
    env_steps: int
    env_cfg: EnvConfig
    board: Board = None
    units: Dict[str, Dict[str, Unit]] = field(default_factory=dict)
    factories: Dict[str, Dict[str, Factory]] = field(default_factory=dict)
    teams: Dict[str, Team] = field(default_factory=dict)
    global_id: int = 0

    def generate_unit_data(units_dict):
        units = dict()
        for team in units_dict:
            units[team] = dict()
            for unit in units_dict[team].values():
                state_dict = unit.state_dict()
                # if self.env_cfg.UNIT_ACTION_QUEUE_SIZE == 1:
                #     # if config is such that action queue is size 1, we do not include the queue as it is always empty
                #     del state_dict["action_queue"]
                units[team][unit.unit_id] = state_dict
        return units
    def generate_team_data(teams_dict):
        teams = dict()
        for k, v in teams_dict.items():
            teams[k] = v.state_dict()
        return teams
    def generate_factory_data(factories_dict):
        factories = dict()
        for team in factories_dict:
            factories[team] = dict()
            for factory in factories_dict[team].values():
                state_dict = factory.state_dict()
                factories[team][factory.unit_id] = state_dict
        return factories
    
    def get_obs(self):
        
        # TODO: speedups?
        units = State.generate_unit_data(self.units)
        teams = State.generate_team_data(self.teams)
        factories = State.generate_factory_data(self.factories)
        board = self.board.state_dict()
        return dict(
            units=units,
            team=teams,
            factories=factories,
            board=board
        )
    def get_compressed_obs(self):
        # return everything on turn 0
        if self.env_steps == 0:
            return self.get_obs()
        else:
            data = self.get_obs()
            # convert lichen and lichen strains to sparse matrix format?
            del data["board"]["ore"]
            del data["board"]["ice"]
            return data
    def from_obs(obs):
        # generate state from compressed obs
        pass