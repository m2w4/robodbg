from setuptools import setup, find_packages
from setuptools.dist import Distribution


class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True


setup(
    name="robodbg",
    version="0.0.2",
    description="A debugger interface for Windows",
    author="Milkshake",
    author_email="friday-font-guru@duck.com",
    license="MIT",
    package_dir={"": "src"},
    packages=find_packages(where="src"),
    include_package_data=True,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Environment :: Win32 (MS Windows)",
    ],
    python_requires=">=3.11,<3.14",
    zip_safe=False,
    distclass=BinaryDistribution,
)
