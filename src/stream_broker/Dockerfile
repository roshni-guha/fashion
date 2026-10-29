FROM golang:1.27-alpine AS build

WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN go build -o broker .

FROM alpine:3.22
WORKDIR /app
COPY --from=build /app/broker /app/index.html ./

# 8090: HTTP signaling + UI, 8443/udp: WebRTC media/data (ICE UDP mux)
EXPOSE 8090 8443/udp
CMD ["./broker"]
