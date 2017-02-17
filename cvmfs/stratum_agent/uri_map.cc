/**
 * This file is part of the CernVM File System
 */

#include "cvmfs_config.h"
#include "uri_map.h"

#include <cassert>

using namespace std;  // NOLINT

WebRequest::WebRequest() : verb_(kUnknown) { }

WebRequest *WebRequest::Create(struct mg_request_info *req) {
  WebRequest *request = new WebRequest();
  string method = req->request_method;
  if (method == "GET")
    request->verb_ = WebRequest::kGet;
  else if (method == "PUT")
    request->verb_ = WebRequest::kPut;
  else if (method == "POST")
    request->verb_ = WebRequest::kPost;
  else if (method == "DELETE")
    request->verb_ = WebRequest::kDelete;
  else
    request->verb_ = WebRequest::kUnknown;
  request->uri_ = req->uri;
  return request;
}


//------------------------------------------------------------------------------


void UriMap::Register(const string &uri_spec, UriHandler *handler) {
  Pathspec path_spec(uri_spec);
  assert(path_spec.IsValid() && path_spec.IsAbsolute());
  rules_.push_back(Match(path_spec, handler));
}


UriHandler *UriMap::Route(const std::string &uri) {
  for (unsigned i = 0; i < rules_.size(); ++i) {
    if (rules_[i].uri_spec.IsPrefixMatching(uri))
      return rules_[i].handler;
  }
  return NULL;
}
