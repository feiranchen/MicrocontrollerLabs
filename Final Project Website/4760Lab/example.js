// First, checks if it isn't implemented yet.
if (!String.prototype.format) {
  String.prototype.format = function() {
    var args = arguments;
    return this.replace(/{(\d+)}/g, function(match, number) { 
      return typeof args[number] != 'undefined'
        ? args[number]
        : match
      ;
    });
  };
}



var http = require('http');
var url=require('url');
var fs = require('fs');
var filename = './samplePureVector';
var querystring = require('querystring');
var express = require('express');
var compression = require('compression');
var app = express();


app.use(express.static(__dirname));

app.set('port', process.env.PORT);
// app.listen(app.get('port'), function(){
//   console.log('Express server listening on port ' + app.get('port'));
// });

http.createServer(app).listen(app.get('port'), function(){
  console.log('Express server listening on port ' + app.get('port'));
});

function processPost(request, response, callback) {
    var queryData = "";
    if(typeof callback !== 'function') return null;

    if(request.method == 'POST') {
        request.on('data', function(data) {
            queryData += data;
            if(queryData.length > 1e6) {
                queryData = "";
                response.writeHead(413, {'Content-Type': 'text/plain'}).end();
                request.connection.destroy();
            }
        });

        request.on('end', function() {
            request.post = querystring.parse(queryData);
            callback();
        });

    } else {
        response.writeHead(405, {'Content-Type': 'text/plain'});
        response.end();
    }
}


// http.createServer(function (req, res) {
// 	var pathname=url.parse(req.url).pathname;
// 	fs.readFile('./index.html', function (err, html) {
// 	    if (err) {
// 	        throw err; 
// 	    }
//         // console.log(html);
//         res.writeHeader(200);//, {"Content-Type": "text/html"});  
//         res.write(html);  
//         res.end();
//     });
// 	// if(req.method == 'POST') {
//  //        processPost(req, res, function() {
//  //            console.log(req.post);
//  //            // Use req.post here

//  //            res.writeHead(200, "OK", {'Content-Type': 'text/plain'});
//  //            res.end();
//  //        });
//  //    } else {
//  //        res.writeHead(200, "OK", {'Content-Type': 'text/plain'});
// 	// 	res.end('Hello World\n');
//  //    }
// }).listen(1337, '127.0.0.1');
// console.log('Server running at http://127.0.0.1:1337/');

var gerber = "";
var gerber_lines;

function readGerber() {
	fs.readFile(filename, 'utf8', function (err,data) {
	  if (err) {
	    return console.log(err);
	  }
	  gerber = data+"";
	  gerber_lines = gerber.split('\n');
	  console.log(gerber_lines);
	});
}

function writeLine(text){
    fs.writeFile("/tmp/test", text, function(err) {
    if(err) {
        console.log(err);
    } else {
        console.log("The file was saved!");
    }
}); 
}

readGerber();

/*
baudrate: Baud Rate, defaults to 9600. 
databits: Data Bits, defaults to 8.
stopbits: Stop Bits, defaults to 1.
parity: Parity, defaults to 'none'.
buffersize: Size of read buffer, defaults to 255.

*/

/*
var SerialPort = require("serialport").SerialPort;
var serialPort = 
new SerialPort("/dev/cu.usbserial-A603UZG1", {
  baudrate: 9600
});
console.log('opened serialport');


var inputBuffer = "";
var line = "";
var lineIndex = 0;

function trimBuffer(){
	// console.log('remaining' + inputBuffer + "<End>")
	var lineEnding = inputBuffer.indexOf('\r\n');
	if (lineEnding > -1){
		line = inputBuffer.substring(0, lineEnding);
		inputBuffer = inputBuffer.slice(lineEnding + 2)
		console.log('got a line' + line + "<End>");
		console.log('Buffer trimmed to' + inputBuffer + '<End>')
	} else {
		line = ""
	}
}
serialPort.on("open", function () {
	serialPort.on('data', function(data) {
		console.log("Data!!"+data);
		console.log("Printing buffer" +inputBuffer);
		inputBuffer += data;
		while(inputBuffer.indexOf('\r\n') > -1){
			trimBuffer();
			// This is to discard the noise charactors
			line = line.substring(line.length-11)
			
			console.log('line is' + line + "<End>\nshouldbFile Length")
			console.log(line.length + "length \n" + "File Length".length)
			console.log(line == "File Length\r")
			if (line == "File Length") {
				line = "";
				console.log("File Length!!\n");
				serialPort.write("{0}*\r\n".format(gerber_lines.length), function(err, results) {
					serialPort.drain(function(){
						console.log("Lenth Written:" + gerber_lines.length);
						lineIndex = 0;
					});
					//console.log('err ' + err);
					//console.log('results ' + results);
				});
			} else if (line == "Hi") {
				line = "";
				if(gerber == "") {
					console.log("File loading");
					return;
				}
				//console.log('Received Hi\r\n');
				serialPort.write(gerber_lines[lineIndex]+"*\r\n", function(err, results) {
				//serialPort.write("test\r\n", function(err, results) {
					serialPort.drain(function(){
						console.log("Written."+ gerber_lines[lineIndex]+"\r\n");
					});
					lineIndex++;
					console.log("sent"+lineIndex +": "+ gerber_lines[lineIndex]+"<End of MSG>");
					//console.log('err ' + err);
					//console.log('results ' + results);
				});
			}
		}
	});
});
*/

/* This is too fast
serialPort.on("open", function () {
	serialPort.on('data', function(data) {
		console.log("Data!!");
		inputBuffer += data;
		if (inputBuffer.indexOf('*')> -1){
			console.log("DeletingBuffer");
			trimBuffer();
			return;
		}
		trimBuffer();
		
		//console.log('data received: ' + data);
		if (line == "File Length") {
			line = "";
			console.log("File Length!!\n");
			serialPort.write("{0}*\r\n".format(gerber_lines.length), function(err, results) {
				serialPort.drain(function(){
					console.log("Lenth Written:" + gerber_lines.length);
				});
				//console.log('err ' + err);
				//console.log('results ' + results);
			});
			if(gerber == "") {
				console.log("File loading");
				return;
			}

			for (var i = 0; i< gerber_lines.length; i++){
				//serialPort.write(gerber_lines[lineIndex]+"\0", function(err, results) {
				serialPort.write("test\r\n", function(err, results) {
					serialPort.drain(function(){
						console.log("Written.");
					});
					lineIndex++;
					console.log("sent"+lineIndex +": "+ gerber_lines[lineIndex]+"====");
					//console.log('err ' + err);
					//console.log('results ' + results);
				});
			}
		}
	});
});
*/

// This works
/*
serialPort.on("open", function () {
	serialPort.on('data', function(data) {
		console.log('data received: ' + data);
		serialPort.write("test\r\n", function(err, results) {
			console.log(gerber.length)
			console.log('err ' + err);
			console.log('results ' + results);
		});
	});
*/











	// console.log('open');
 //  	console.log(gerber);

 //  	remaining = gerber;
 //  	var index = remaining.indexOf('\n');
 //  	var last = 0;
 //  	while (index > -1){
 //  		var line = remaining.substring(last, index);
 //  		last = index + 1;
 //  		serialPort.write(line, function(err, results) {
	// 		console.log('err ' + err);
	// 		console.log('results ' + results);
	// 	});
 //  	}
	