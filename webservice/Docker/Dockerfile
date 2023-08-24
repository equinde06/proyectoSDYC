# Utiliza una imagen base apropiada, por ejemplo, un servidor web como nginx
FROM nginx:alpine

# Copia los archivos de tu proyecto al contenedor
COPY index.html /usr/share/nginx/html/
COPY main.js /usr/share/nginx/html/

# Puerto en el que el servidor web en el contenedor estará escuchando
EXPOSE 80

# Comando para iniciar el servidor web
CMD ["nginx", "-g", "daemon off;"]
