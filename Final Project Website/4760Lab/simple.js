var http = require('http');
var connect = require('connect');
fs = require('fs')

console.log('\n\n--- Node Version: ' + process.version + ' ---');

var serveStatic = require('serve-static');

// Set up Connect routing
var app = connect()
    .use(serveStatic('./'))
    .use(function(req, res) {
        console.log('Could not find handler for: ' + req.url);
        res.end('Could not find handler for: ' + req.url);
    })
    .use(function(err, req, res, next) {
        console.log('Error trapped by Connect: ' + err.message + ' : ' + err.stack);
        res.end('Error trapped by Connect: ' + err.message);
    });
// app.use(serveStatic('public')).listen(8080, "0.0.0.0");
// Start node server listening on specified port -----
http.createServer(app).listen(8080);

console.log('HTTP server listening on port 8080');


/*
fs.readFile('./circuitPad.html', 'utf8', function (err,html) {
  if (err) {
    throw err;
  }
  http.createServer(function(request, response) {  
        response.writeHeader(200, {"Content-Type": "text/html"});  
        response.write(html);  
        response.end();  
    }).listen(8000, '128.84.125.20');
console.log('Server running at http://127.0.0.1:8000/');
});*/