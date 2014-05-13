<?php

class OAuth {
    
    protected $_client_id;
    protected $_client_secret;
    protected $_callback;
    
    protected $_access_token;
    protected $_access_token_secret;
    protected $_expires;
    
    protected $_scope;
    
    protected $_prefix;
    protected $_authorize_url;
    protected $_access_token_url;
    protected $_request_token_url;
    
    protected function __construct($params) {
        $this->_client_id = $params['id'];
        $this->_client_secret = $params['secret'];
        $this->_callback = $params['callback'];
        if (isset($params['scope'])) {
            $this->_scope = $params['scope'];
        }
    }

    public static function getInstance($platform) {
        $scheme = OAuth::url_scheme();
        $port = $_SERVER['SERVER_PORT'];
        // load params from conf.inc
        // id, secret are necessary
        $params = Conf::$oauth[$platform];

        if (!isset($params) || !isset($params['id']) || !isset($params['secret']) || !class_exists($platform))
            return false;
        
        // fix port
        if( $scheme == "https" && $port == 443 || $scheme == "http" && $port == 80 ) {
            $port = "";
        } else { $port = ":" . $port; }

        $params['callback'] = "$scheme://$_SERVER[HTTP_HOST]$port/oauth/callback?platform=$platform";
        return new $platform($params);
    }
    
    private static function url_scheme() {
        /* apache */
        if (isset($_SERVER['HTTPS']) && ($_SERVER['HTTPS'] === 'on' || $_SERVER['HTTPS'] == 1)) return 'https';
        /* nginx */
        if (isset($_SERVER['SERVER_PORT']) && ($_SERVER['SERVER_PORT'] === '443')) return 'https';
        
        return "http";
    }

    public function setAccessToken($access_token, $access_token_secret = null, $expires = null) {
        $this->_access_token = $access_token;
        $this->_access_token_secret = $access_token_secret;
        $this->_expires = $expires;
    }
    
    public function makeRequest($url, Array $parameters = array(), $method = 'GET', $returnType = 'json', $includeCallback = false, $includeVerifier = false) {

        // check if current token has expired
        if(isset($_SESSION[$this->_prefix]['expires']) && $_SESSION[$this->_prefix]['expires'] < time()) {
            unset($_SESSION[$this->_prefix]);
            $this->authorize($this->_scope);
            return false;
        }

        // set oauth headers for oauth 1.0
        if(isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
            $headers = $this->getOAuthHeaders($includeCallback);
            if($includeVerifier && isset($_GET['oauth_verifier'])) {
                $headers['oauth_verifier'] = $_GET['oauth_verifier'];
            }
            $base_info = $this->buildBaseString($url, $method, $headers);
            $composite_key = $this->getCompositeKey();
            $headers['oauth_signature'] = base64_encode(hash_hmac('sha1', $base_info, $composite_key, true));
            $header = array($this->buildAuthorizationHeader($headers), 'Expect:');
        }
        // add access token to parameter list
        else if(isset($_SESSION[$this->_prefix]['access_token'])) {
            $parameters = array_merge($parameters, $this->getAttachParameters());            
        }
        
        // create a querystring for GET requests
        if(count($parameters) > 0 && $method == 'GET' && strpos($url, '?') === false) {
            $p = array();
            foreach($parameters as $k => $v) {
                $p[] = $k . '=' . $v;
            }
            $querystring = implode('&', $p);
            $url = $url . '?' . $querystring;
        }
        
        // set default CURL options
        $options = array(
            CURLOPT_URL => $url,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_ENCODING => 'gzip'
        );
        
        // set CURL headers for oauth 1.0 requests
        if (isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
            $options[CURLOPT_HTTPHEADER] = $header;
            $options[CURLOPT_HEADER] = false;
        }
        
        // set post fields for POST requests
        if($method == 'POST') {
            $options[CURLOPT_POST] = true;
            $options[CURLOPT_POSTFIELDS] = http_build_query($parameters);
        }
        
        $options[CURLOPT_USERAGENT] = $this->getUserAgent();

        // make CURL request
        $curl = curl_init();
        curl_setopt_array($curl, $options);
        $response = curl_exec($curl);
        $info = curl_getinfo($curl);
        curl_close($curl);
        
        // show error when http_code is not 200
        if($info['http_code'] != 200) {
            // mostly errors are thrown when a user has denied access
            unset($_SESSION[$this->_prefix]);
            throw new Exception($response);
        }
        
        // return json decoded array or plain response
        if($returnType == 'json') {
            return json_decode($response, true);
        } else {
            return $response;
        }
    }
    
    public function validateAccessToken() {
        // return true if access token is found
        if(isset($_SESSION[$this->_prefix]['access_token']) || (isset($this->_access_token) && strlen($this->_access_token) > 0)) {
            $this->_access_token = $_SESSION[$this->_prefix]['access_token'];
            if(isset($_SESSION[$this->_prefix]['access_token_secret'])) {
                $this->_access_token_secret = $_SESSION[$this->_prefix]['access_token_secret'];
            }
            if(isset($_SESSION[$this->_prefix]['expires'])) {
                $this->_expires = $_SESSION[$this->_prefix]['expires'];
            }
            return true;
        }
        // authorize app if no token is found
        if(!isset($this->_access_token) || strlen($this->_access_token) == 0) {
            // handle oauth 1.0 flow
            if(isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
                // request token and authorize app
                if(!isset($_GET['oauth_token']) && !isset($_GET['oauth_verifier'])) {
                    $this->requestToken();
                    $this->authorize();
                    return false;
                }
                // request access token
                else {
                    if($_GET['oauth_token'] != $_SESSION[$this->_prefix]['token']) {
                        unset($_SESSION[$this->_prefix]['token'], $_SESSION[$this->_prefix]['token_secret']);
                        return false;
                    } else {
                        $this->requestAccessToken();
                        unset($_SESSION[$this->_prefix]['token'], $_SESSION[$this->_prefix]['token_secret']);
                        $this->_access_token = $_SESSION[$this->_prefix]['access_token'];
                        $this->_access_token_secret = $_SESSION[$this->_prefix]['access_token_secret'];
                        return true;
                    }
                }
            }
            // handle oauth 2.0 flow
            else {
                // authorize app
                if(!isset($_GET['state']) && !isset($_GET['code'])) {
                    $this->authorize($this->_scope);
                    return false;
                }
                // request access token
                else {
                    if($_GET['state'] != $_SESSION[$this->_prefix]['state']) {
                        unset($_SESSION[$this->_prefix]['state']);
                        return false;
                    } else {
                        unset($_SESSION[$this->_prefix]['state']);
                        $this->requestAccessToken();
                        return true;
                    }
                }
            }
        }
    }
    
    protected function requestToken($returnType = 'flat', Array $values = array('oauth_token', 'oauth_token_secret')) {
        // make the request
        $response = $this->makeRequest($this->_request_token_url, array(), 'POST', $returnType, true);
        
        // get the correct parameters from the response
        $params = $this->getParameters($response, $returnType);
        
        // add the token and token secret to the session
        if(isset($params[$values[0]]) && isset($params[$values[1]])) {
            $_SESSION[$this->_prefix]['token'] = $params[$values[0]];
            $_SESSION[$this->_prefix]['token_secret'] = $params[$values[1]];
        }
        // throw exception if incorrect parameters were returned
        else {
            $s = '';
            foreach($params as $k => $v) {$s = $k . '=' . $v;}
            throw new Exception('incorrect access token parameters returned: ' . implode('&', $s));
        }
    }
    
    protected function requestAccessToken($method = 'GET', Array $params = array(), $returnType = 'flat', Array $values = array('access_token', 'expires')) {
        // add oauth verifier to parameters for oauth 1.0 request
        if(isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
            $parameters = array('oauth_verifier' => $_GET['oauth_verifier']);
        }
        // set parameters for oauth 2.0 request
        else {
            $parameters = array(
                'client_id' => $this->_client_id,
                'redirect_uri' => $this->_callback,
                'client_secret' => $this->_client_secret,
                'code' => $_GET['code']
            );
        }
        $parameters = array_merge($parameters, $params);

        // make the request
        $response = $this->makeRequest($this->_access_token_url, $parameters, $method, $returnType, false);
        // get the correct parameters from the response
        $params = $this->getParameters($response, $returnType);
        
        // add the token to the session
        // FIXME 第二个参数，不同平台返回的值是不同的，可能需要重载（处理过期问题）
        if(isset($params[$values[0]])) {
            $_SESSION[$this->_prefix]['access_token'] = $params[$values[0]];
            if(isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
                $_SESSION[$this->_prefix]['access_token_secret'] = $params[$values[1]];
            } else if ($values[1] != 'no_expires') {
                $_SESSION[$this->_prefix]['expires'] = time() + $params[$values[1]];
            }
        }
        // throw exception if incorrect parameters were returned
        else {
            $s = '';
            foreach($params as $k => $v) {$s = $k . '=' . $v;}
            throw new Exception('incorrect access token parameters returned: ' . implode('&', $s));
        }
    }
    
    protected function authorize($scope = [], $scope_seperator = ',', $attach = null) {
        // build authorize url for oauth 1.0 requests
        if(isset($this->_request_token_url) && strlen($this->_request_token_url) > 0) {
            $this->_authorize_url .= '?oauth_token=' . $_SESSION[$this->_prefix]['token'];
        }
        // build authorize url for oauth 2.0 requests
        else {
            $this->_authorize_url .= '?client_id=' . $this->_client_id . '&redirect_uri=' . $this->_callback;
            $state = md5(time() . mt_rand());
            $_SESSION[$this->_prefix]['state'] = $state;
            $this->_authorize_url .= '&state=' . $state . '&scope=' . implode($scope_seperator, $scope) . $attach;
        }
        // redirect
        header('Location: ' . $this->_authorize_url);exit;
    }

    protected function getUserAgent() {
        return "";
    }
    
    protected function getAttachParameters() {
        return ['access_token' => $_SESSION[$this->_prefix]['access_token']];
    }

    private function getParameters($response, $returnType) {
        if($returnType != 'json') {
            $r = explode('&', $response);
            $params = array();
            foreach($r as $v) {
                $param = explode('=', $v);
                $params[$param[0]] = $param[1];
            }
        } else {
            $params = $response;
        }
        return $params;
    }
    
    private function getCompositeKey() {
        if(isset($this->_access_token_secret) && strlen($this->_access_token_secret) > 0) {
            $composite_key = rawurlencode($this->_client_secret) . '&' . rawurlencode($this->_access_token_secret);
        } else if(isset($_SESSION[$this->_prefix]['token_secret'])) {
            $composite_key = rawurlencode($this->_client_secret) . '&' . rawurlencode($_SESSION[$this->_prefix]['token_secret']);
        } else {
            $composite_key = rawurlencode($this->_client_secret) . '&';
        }
        return $composite_key;
    }
    
    private function getOAuthHeaders($includeCallback = false) {
        $oauth = array(
            'oauth_consumer_key' => $this->_client_id,
            'oauth_nonce' => time(),
            'oauth_signature_method' => 'HMAC-SHA1',
            'oauth_timestamp' => time(),
            'oauth_version' => '1.0'
        );
        if(isset($this->_access_token)) {
            $oauth['oauth_token'] = $this->_access_token;
        } else if(isset($_SESSION[$this->_prefix]['token'])) {
            $oauth['oauth_token'] = $_SESSION[$this->_prefix]['token'];
        }
        if($includeCallback) {
            $oauth['oauth_callback'] = $this->_callback;
        }
        return $oauth;
    }
    
    private function buildBaseString($baseURI, $method, $params) {
        $r = array();
        ksort($params);
        foreach($params as $key => $value) {
            $r[] = $key . '=' . rawurlencode($value);
        }
        return $method . '&' . rawurlencode($baseURI) . '&' . rawurlencode(implode('&', $r));
    }
    
    private function buildAuthorizationHeader($oauth) {
        $r = 'Authorization: OAuth ';
        $values = array();
        foreach($oauth as $key => $value) {
            $values[] = $key . '="' . rawurlencode($value) . '"';
        }
        $r .= implode(', ', $values);
        return $r;
    }
    
}

class Facebook extends OAuth {
    // app: https://developers.facebook.com/
    // api: https://developers.facebook.com/docs/
        
    protected $_prefix = 'oauth_facebook';
    protected $_authorize_url = 'https://www.facebook.com/dialog/oauth';
    protected $_access_token_url = 'https://graph.facebook.com/oauth/access_token';
        
}

class Google extends OAuth {
    // app: https://console.developers.google.com/project
    // api: https://developers.google.com/accounts/docs/OAuth2WebServer
        
    protected $_prefix = 'oauth_google';
    protected $_authorize_url = 'https://accounts.google.com/o/oauth2/auth';
    protected $_access_token_url = 'https://accounts.google.com/o/oauth2/token';
    
    protected function authorize($scope = array(), $scope_seperator = '+') {
        parent::authorize($scope, $scope_seperator, '&response_type=code');
    }
    
    protected function requestAccessToken() {
        parent::requestAccessToken('POST', ['grant_type' => 'authorization_code'], 'json', ['access_token', 'expires_in']);
    }
        
}

class StackExchange extends OAuth {
    // app: http://stackapps.com/apps/oauth
    // api: https://api.stackexchange.com/docs

    protected $_prefix = 'oauth_stackexchange';
    protected $_authorize_url = 'https://stackexchange.com/oauth';
    protected $_access_token_url = 'https://stackexchange.com/oauth/access_token';
    protected $_key = '';

    protected function __construct($params) {
        $this->_key = isset($params['key']) ? $params['key'] : "";
        parent::__construct($params);
    }

    protected function requestAccessToken() {
        parent::requestAccessToken('POST', ['grant_type' => 'authorization_code']);
    }

    protected function getAttachParameters() {
        $params = [];
        $params['access_token'] = $_SESSION[$this->_prefix]['access_token'];
        $params['key'] = $this->_key;
        return $params;
    }

}

class GitHub extends OAuth {
    // app: https://github.com/settings/applications
    // api: https://developer.github.com/v3/oauth/
        
    protected $_prefix = 'oauth_github';
    protected $_authorize_url = 'https://github.com/login/oauth/authorize';
    protected $_access_token_url = 'https://github.com/login/oauth/access_token';
    protected $_app_name = '';

    protected function __construct($params) {
        $this->_app_name = isset($params['appname']) ? $params['appname'] : $params['id'];
        parent::__construct($params);
    }
    
    protected function requestAccessToken() {
        parent::requestAccessToken('POST', [], 'flat', ['access_token', 'no_expires']);
    }

    protected function getUserAgent() {
        return $this->_app_name;
    }

}

class LinkedIn extends OAuth {
    // app: https://www.linkedin.com/secure/developer
    // api: https://developer.linkedin.com/apis
        
    protected $_prefix = 'oauth_linkedin';
    protected $_authorize_url = 'https://www.linkedin.com/uas/oauth2/authorization';
    protected $_access_token_url = 'https://www.linkedin.com/uas/oauth2/accessToken';

    protected function authorize($scope = []) {
        parent::authorize($scope, ' ', '&response_type=code');
    }

    protected function requestAccessToken() {
        parent::requestAccessToken('POST', ['grant_type' => 'authorization_code'], 'json', ['access_token', 'expires_in']);
    }
    
    protected function getAttachParameters() {
        $params = [];
        $params['oauth2_access_token'] = $_SESSION[$this->_prefix]['access_token'];
        $params['format'] = 'json';
        return $params;
    }

}

class Weibo extends OAuth {
    // oauth: http://open.weibo.com/wiki/%E6%8E%88%E6%9D%83%E6%9C%BA%E5%88%B6
    // api: http://open.weibo.com/wiki/%E5%BE%AE%E5%8D%9AAPI
    // app: http://open.weibo.com/webmaster

    protected $_prefix = 'oauth_weibo';
    protected $_authorize_url = 'https://api.weibo.com/oauth2/authorize';
    protected $_access_token_url = 'https://api.weibo.com/oauth2/access_token';

    protected function requestAccessToken() {
        parent::requestAccessToken('POST', ['grant_type' => 'authorization_code'], 'json', ['access_token', 'expires_in']);
    }
}

class Twitter extends OAuth {
    // app: https://apps.twitter.com/
    // api: https://dev.twitter.com/docs

    protected $_prefix = 'oauth_twitter';
    protected $_authorize_url = 'https://api.twitter.com/oauth/authorize';
    protected $_access_token_url = 'https://api.twitter.com/oauth/access_token';
    protected $_request_token_url = 'https://api.twitter.com/oauth/request_token';
    
    protected function requestAccessToken() {
        parent::requestAccessToken('POST', [], 'flat', ['oauth_token', 'oauth_token_secret']);
    }

}
