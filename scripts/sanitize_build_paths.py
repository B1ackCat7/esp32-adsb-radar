"""Avoid embedding developer machine paths in redistributable firmware."""
Import('env')
from pathlib import Path
from platformio.project.config import ProjectConfig

project = str(Path(env.subst('$PROJECT_DIR')).resolve())
core = str(Path(ProjectConfig.get_instance().get('platformio', 'core_dir')).expanduser().resolve())
# GCC applies these mappings to __FILE__ strings as well as debug information.
# Read paths from the environment; never commit a developer's absolute directory.
env.Append(CCFLAGS=[f'-ffile-prefix-map={project}=project',
                    f'-ffile-prefix-map={core}=platformio'])
