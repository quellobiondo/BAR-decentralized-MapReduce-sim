ARG VERSION=3.11.1
FROM simgrid:$VERSION

WORKDIR /home/experiment/

COPY ./platform /home/experiment/platform
COPY ./experiments /home/experiment/experiments

WORKDIR experiments

COPY ./Tools/entrypoint.sh /home/experiment/experiments

RUN make clean all

ENV LD_LIBRARY_PATH /usr/local/lib/

ENTRYPOINT [ "./entrypoint.sh" ]
