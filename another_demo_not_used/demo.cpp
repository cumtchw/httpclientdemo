if (!m_httpclient->getData(m_userinfo.m_cHttpServerHost, m_userinfo.m_nHttpServerPort, szPath, szGetContent, response))
 {
  //失败直接返回response
  response = _produceResponseJson(false, "getData failed");
  return false;
 }

 if (!m_httpclient->parse_body(response, returncode, body, bodylen))
 {
  //解析失败直接返回response
  response = _produceResponseJson(false, "parse_body failed");
  return false;
 }









 