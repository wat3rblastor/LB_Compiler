FROM gcc:11.2.0-bullseye

ARG UID=1000
ARG GID=1000
ARG USERNAME=dev

ENV DEBIAN_FRONTEND=noninteractive

# Install only the tools required to build, test, and benchmark the compilers.
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    bash \
    make \
    ncurses-bin \
    time \
    valgrind \
  && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid "${GID}" "${USERNAME}" \
  && useradd --uid "${UID}" --gid "${GID}" --create-home --shell /bin/bash "${USERNAME}" \
  && rm -rf /var/lib/apt/lists/*

# Build the repo after container startup (do not compile during image build).
# If build errors report missing headers/libs, install the matching apt package(s) above.
WORKDIR /workspace
USER ${USERNAME}
