from setuptools import Extension, setup
from Cython.Build import cythonize
from pathlib import Path

CASYNCIO_PATH = Path(__file__).parent / "multifuture"
print(CASYNCIO_PATH)
if __name__ == "__main__":
    setup(
        ext_modules=cythonize(
            Extension(
                "multifuture.multifuture",
                ["multifuture/multifuture.pyx"],
                include_dirs=[str(CASYNCIO_PATH)],
            ),
            cache=True,
        )
    )
