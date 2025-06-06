  foreach(z_vcpkg_curl_component IN ITEMS SSL IPv6 UnixSockets libz AsynchDNS Largefile alt-svc HSTS NTLM TLS-SRP HTTPS-proxy threadsafe DICT FILE FTP FTPS GOPHER GOPHERS HTTP HTTPS IMAP IMAPS IPFS IPNS MQTT POP3 POP3S RTSP SMB SMBS SMTP SMTPS TELNET TFTP)
    if(z_vcpkg_curl_component MATCHES "^[-_a-zA-Z0-9]*$")
      set(CURL_${z_vcpkg_curl_component}_FOUND TRUE)
    endif()
  endforeach()
  