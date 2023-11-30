from dataclasses import dataclass

import marshmallow.exceptions
from marshmallow import Schema
from marshmallow import fields
from marshmallow import post_load
from marshmallow import validates_schema


ALLOWED_COMMANDS = [
    "UP",
    "IDLE_US",
    "DOWN",
]

@dataclass
class Program:
    version: str
    program_body: dict


class Command(Schema):
    command = fields.String()
    args = fields.List(fields.Float())


class ProgramBody(Schema):
    commands_len = fields.Integer(required=True)
    commands_list = fields.List(fields.Nested(Command))


class ProgramSchema(Schema):
    version = fields.String(required=True)
    program_body = fields.Nested(ProgramBody)

    @validates_schema
    def check_commands(self, obj, *args, **kwargs) -> None:
        for command in obj['program_body']['commands_list']:
            if (command_name := command['command']) not in ALLOWED_COMMANDS:
                msg = f'Command: "{command_name}" is not allowed.'
                raise marshmallow.exceptions.ValidationError(msg)
        return obj


    @post_load
    def deserialize_to_dataclass(self, obj, *args, **kwargs) -> Program:
        return Program(**obj)



