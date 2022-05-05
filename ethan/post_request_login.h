void post_method(char* user,char* password,char* function) {
    char body[100]; 
    sprintf(body,"user=%s,password=%s,function=%s",user,password,function);
    int body_len = strlen(body); 
    sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/team33/final/request_handler_login.py, HTTP/1.1\r\n");
    strcat(request_buffer,"Host: team33@608dev-2.net\r\n");
    strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); 
    strcat(request_buffer,"\r\n"); 
    strcat(request_buffer,body); 
    strcat(request_buffer,"\r\n"); 
    do_http_request("team33@608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
    Serial.println(response_buffer); 
}