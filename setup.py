from setuptools import setup, find_packages

with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()

with open('requirements.txt', 'r', encoding='utf-8') as f:
    requirements = [line.strip() for line in f if line.strip()]

setup(
    name='gpt4all',
    version='0.0.1',
    author='nomic-ai',
    author_email='zach@nomic-ai',
    description='an ecosystem of open-source chatbots trained on a massive collections of clean assistant data including code, stories and dialogue',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/nomic-ai/gpt4all',
    packages=find_packages(),
    install_requires=requirements,
    classifiers=[
        'Development Status :: 3 - Alpha',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Topic :: Text Processing :: Linguistic',
        'Topic :: Scientific/Engineering :: Artificial Intelligence',
        'Intended Audience :: Science/Research',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
)