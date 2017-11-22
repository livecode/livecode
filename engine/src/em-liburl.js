/*                                                              -*-Javascript-*-

Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

mergeInto(LibraryManager.library, {

    $LiveCodeLibURL: {
        requestMap: null,
        requestCount: 1,
        libURLStack: null,

        init: function() {
            this.requestMap = new Map();
            this.libURLStack = document.liveCode.findStackWithName('revliburl');
            if (typeof this.libURLStack.ulExtXMLHTTPCallback != 'function') {
                return 'Unable to find callback handler.'
            }
        },

        isValidRequest: function(request) {
            return this.requestMap.has(request);
        },

        requestCreate: function(url, method, async) {
            var xhr = new XMLHttpRequest();

            // store any additional meta against the request that we need
            xhr.url = url;
            xhr.async = async;
            xhr.responseBase64 = null;
            xhr.requestHeaders = new Map();

            // if possible, get the raw data without any char set converions etc
            // as per the xhr spec, this is only possible for asynchronous requests
            // for synchronous requests, we can override the mime type forceing
            // the browser to return the URL in the x-user-defined charset which
            // can be parsed as a stream of binary data
            if (async) {
                xhr.responseType = 'blob';
            } else {
                xhr.overrideMimeType('text\/plain; charset=x-user-defined');
            }

            xhr.addEventListener('loadstart', this.requestCallbackLoadStart);
            xhr.addEventListener('progress', this.requestCallbackProgress);
            xhr.addEventListener('abort', this.requestCallbackAbort);
            xhr.addEventListener('error', this.requestCallbackError);
            xhr.addEventListener('load', this.requestCallbackLoad);
            xhr.addEventListener('timeout', this.requestCallbackTimeout);
            xhr.addEventListener('loadend', this.requestCallbackLoadEnd);

            try {
                xhr.open(method, url, async);
            } catch(error) {
                return 'Error opening request: ' + error.message;
            }

            // assign each request an id that will be used as a handle by liburl
            xhr.key = this.requestCount++;
            this.requestMap.set(xhr.key, xhr);
            return xhr.key;
        },
        requestDestroy: function(request) {
            if (this.requestMap.has(request)) {
                this.requestMap.delete(request);
            }
        },

        requestSend: function(request, body) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                try {
                    xhr.send(body);

                    // at this point, synchronous requests will have completed
                    // if the http status is not 2xx then assume something has
                    // gone wrong and return the error
                    if (!xhr.async && (xhr.status < 200 || xhr.status > 299)) {
                        if (xhr.statusText != null) {
                            return xhr.statusText;
                        } else {
                            return 'Request failed ' + xhr.status;
                        }
                    }
                } catch (error) {
                    return 'Error sending request: ' + error.message;
                }
            }
        },
        requestCancel: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                xhr.abort();
                this.requestDestroy(request);
            }
        },

        requestGetRequestHeaders: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);

                // each browser sends a different set of request headers
                // the xhr API provides no way to access them
                // the user agent is set consitently accross all browsers,
                // so return that along with any headers we set
                var headers = 'User-Agent:' + navigator.userAgent;
                xhr.requestHeaders.foreach(function(headerName, headerValue, map) {
                    if (headers != '') {
                        headers += '\n';
                    }
                    headers += headerName + ':' + headerValue;
                });
                return headers;
            }
        },
        requestGetResponseHeaders: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                return xhr.getAllResponseHeaders();
            }
        },
        requestGetResponse: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                if (xhr.responseBase64 != null) {
                    return xhr.responseBase64;
                } else if (xhr.responseText != null) {
                    return xhr.responseText;
                }
            }
        },
        requestGetURL: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                return xhr.url;
            }
        },
        requestGetStatus: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                return xhr.statusText;
            }
        },
        requestGetStatusCode: function(request) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                return xhr.status;
            }
        },

        requestSetTimeout: function(request, timeout) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                try {
                    // as part of the xhr spec, the timeout can't be set for
                    // synchronous requests when the current global object is "window"
                    // which in our case is always, which will mean that synchronous
                    // requests will never time out
                    if (xhr.async) {
                        xhr.timeout = timeout;
                    }
                } catch (error) {
                    return 'Error setting timeout: ' + error.message;
                }
            }
        },
        requestSetHeader: function(request, name, value) {
            if (this.requestMap.has(request)) {
                var xhr = this.requestMap.get(request);
                try {
                    xhr.setRequestHeader(name, value);
                    // the xhr API doesn't give access to the headers sent with
                    // with the request, so store them directly
                    xhr.requestHeaders.set(name, value);
                } catch (error) {
                    return 'Error setting header ' + name + ': ' + error.message;
                }
            }
        },

        // there are some situations in which it appears neither the "error",
        // "abort", "timeout" or "load" callbacks will be dispatched
        // use the loadend callback as a fallback to detect this case making the
        // assumption that if triggered, an error has occured
        // if the "error", "abort", "timeout" or "load" callbacks are dispatched
        // then the loadend handler should be cleared
        __requestClearLoadEndCallback: function(xhr) {
            xhr.removeEventListener('loadend', this.requestCallbackLoadEnd, false);
        },
        requestCallbackLoadEnd: function(event) {
            LiveCodeLibURL.__sendCallback(this, event, 'error');
        },

        requestCallbackLoadStart: function(event) {
            LiveCodeLibURL.__sendCallback(this, event, event.type);
        },
        requestCallbackProgress: function(event) {
            LiveCodeLibURL.__sendCallback(this, event, event.type);
        },
        requestCallbackAbort: function(event) {
            LiveCodeLibURL.__requestClearLoadEndCallback(this);
            LiveCodeLibURL.__sendCallback(this, event, event.type);
        },
        requestCallbackError: function(event) {
            LiveCodeLibURL.__requestClearLoadEndCallback(this);
            LiveCodeLibURL.__sendCallback(this, event, event.type);
        },
        requestCallbackLoad: function(event) {
            LiveCodeLibURL.__requestClearLoadEndCallback(this);

            // for non 2xx http codes assume an error has occured and send the
            // error callback to liburl rather than load
            var type = event.type;
            if (this.status < 200 || this.status > 299) {
                type = 'error';
            }

            // we want to return the raw data back to libURL
            // however do as javascript assumes a DOMString and performs the
            // appropriate char set conversions when returning the result
            // thus we base64 encode on this side (and decode in libURL)
            var xhr = this;
            if (this.responseType == 'blob') {
                // the FileReader API allows us to convert blobs to base64
                // the API prefixes base64 data with "data:text/plain;base64,"
                // make sure we chop this off
                var reader = new FileReader();
                reader.onloadend = function() {
                    xhr.responseBase64 = reader.result.substring(
                        'data:text/plain;base64,'.length - 1
                    );
                    LiveCodeLibURL.__sendCallback(xhr, event, type);
                };
                reader.readAsDataURL(this.response);
            } else {
                // if we've not got a binary blob back, then assume the data's
                // charset is "x-user-defined" (as specified in the call to overrideMimeType)
                // this apparently uses the unicode private area 0xF700-0xF7ff
                // discarding the high-order byte from each codepoint allows us
                // to access the data as if it were a binary stream
                // more info here -> http://web.archive.org/web/20071103070418/http://mgran.blogspot.com/2006/08/downloading-binary-streams-with.html
                var byteCount = xhr.responseText.length;
                var bytes = new Uint8Array(byteCount);
                for (var i = 0; i < byteCount; i++) {
                    bytes[i] = xhr.responseText.charCodeAt(i) & 0xff;
                }
                xhr.responseBase64 = btoa(String.fromCharCode.apply(null, bytes));
                LiveCodeLibURL.__sendCallback(xhr, event, type);
            }
        },
        requestCallbackTimeout: function(event) {
            LiveCodeLibURL.__requestClearLoadEndCallback(this);
            LiveCodeLibURL.__sendCallback(this, event, event.type);
        },

        __sendCallback: function(xhr, event, type) {
            var total = -1;
            if (event.lengthComputable) {
                total = event.total;
            }
            this.libURLStack.ulExtXMLHTTPCallback(
                xhr.key,
                xhr.url,
                type,
                event.loaded,
                total
            );
        },
    },

    // merely a placeholder to ensure that LiveCodeLibURL is in scope
    MCEmscriptenLibUrlInitializeJS__deps: ['$LiveCodeLibURL'],
    MCEmscriptenLibUrlInitializeJS: function() {
        return true;
    },

    MCEmscriptenLibUrlFinalizeJS__deps: ['$LiveCodeLibURL'],
    MCEmscriptenLibUrlFinalizeJS: function() {},

});
