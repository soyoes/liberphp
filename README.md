liberPHP
=========

A extremely simple &amp; fast php web framework

# Goals
### Extremely Fast
    * Almost As fast as non-framework to handle JSON request.
    * < 14 ms under 100*10 requests.
    * Test with db query &amp; 7.5kb json response. using ApacheBench

### Extremely Easy to Use
    * Only 1 file, with about 25 functions to remember.

# Features.
### RESTful Dispatching 
    * /{$controller}/{$action}/{$ID}
    * /@{$scheme_name}?{$query_conditions}
### HTML Template & Custom Tags
    * 3~5 times faster then Smarty
    * Only about 10 tags to remember.
    * Define your own tags easily
### Simple &amp; Fast DB access
    * No need to define any model, just provide us a scheme file. we do the rest things for you. 
### Auto RESTful response.
    * No controller, no model, all you need to do is just to define a db scheme and permissions.
### L2 Cache
    * ARC + memcached
### Unix-like Permission check.
    * Check all controller/actions permission with annotation
    * Declare permissions like unix command line.
### Annotations &amp; PHPDoc
    * Support variable custom annotations.
    * Generate PHP Doc automatically
### Filters like Servlet
    * Support filters like Java-Servlet to handle auth / oauth process.
### Auto Unit Test
    * Using csv files to define test cases only.
    * Auto test both JSON and HTML.
### Variable Awesome Tools
    * Mail, CSV, GD2 (Image processing), Binary, S3, SEO, Common OAuth client.

 