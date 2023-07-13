FROM node:18.16
RUN apt-get update && apt-get install git
ARG LAB_BOARD_URL
WORKDIR /app-build
ADD src src/
COPY angular.json package.json tsconfig.json tsconfig.app.json upload_dist.sh ./
RUN npm install -g npm
RUN npm i
RUN npm run build
ENV LAB_BOARD_URL=$LAB_BOARD_URL
RUN chmod +x upload_dist.sh
CMD /app-build/upload_dist.sh
