# Stage 1: Build with Emscripten
FROM emscripten/emsdk:7.0.1 AS builder

WORKDIR /src
COPY include/ include/
COPY src/ src/
COPY bindings/browser/jurischain-bundle-api.js bindings/browser/
COPY Makefile ./

RUN make all

# Stage 2: Serve with unprivileged nginx
FROM nginxinc/nginx-unprivileged:alpine AS serve

COPY --from=builder /src/dist/ /usr/share/nginx/html/dist/
COPY examples/web/index.html /usr/share/nginx/html/
COPY examples/web/style.css /usr/share/nginx/html/
COPY docker/nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 8080

CMD ["nginx", "-g", "daemon off;"]
