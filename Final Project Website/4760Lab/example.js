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
var bodyParser = require('body-parser');
var app = express();


app.use(express.static(__dirname));
app.set('port', process.env.PORT);
// app.listen(app.get('port'), function(){
//   console.log('Express server listening on port ' + app.get('port'));
// });
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));

app.post('/frame',function(req,res){
  var frame=req.body.frame;
  console.log("Frame = "+frame);
  res.end("yes");
  frame += "X-1Y-1D2*\n"
  writeFile(frame);
});

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


// File parameters
var frame_count= 0;
var filename_base="/frame_";
var sendCount = 0;
var loaded = 0;
var gerber = "";
var gerber_lines= [];

function writeFile(text){
    fs.writeFile(__dirname+filename_base + frame_count, text, function(err) {
	    if(err) {
	        console.log(err);
	    } else {
	        console.log("The file was saved!"+frame_count);
	    	frame_count++;
	    	if(!loaded){
	    	  readFile();
	    	}
	    }
	}); 
}

function readFile() {
	if (frame_count <= sendCount){
		loaded = 0;
		return;
	}
	fs.readFile(__dirname+filename_base + sendCount, 'utf8', function (err,data) {
	  if (err) {
	    return console.log(err);
	  }else{
		  if (sendCount<frame_count){
		  	sendCount++;
		  }
		  gerber = data+"";
		  gerber_lines = gerber.split('\n');
		  console.log(gerber_lines);
	  }
	});
}

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

readFile();
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
				console.log("frame" + frame_count + "read" + sendCount);
				line = "";
				console.log("File Length!!\n");
				serialPort.write("{0}*\r\n".format(gerber_lines.length), function(err, results) {
					serialPort.drain(function(){
						console.log("Lenth Written:" + gerber_lines.length);
						lineIndex = 0;
						readFile();
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