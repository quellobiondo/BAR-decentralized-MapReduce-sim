ARG VERSION=3.11.1
FROM simgrid:$VERSION

RUN useradd -ms /bin/bash experiment
USER experiment

WORKDIR /home/experiment/

COPY --chown=experiment ./platform /home/experiment/platform
COPY --chown=experiment ./experiments /home/experiment/experiments

WORKDIR experiments

RUN make clean all

ENV LD_LIBRARY_PATH /usr/local/lib/

CMD [ "/bin/bash" ]
