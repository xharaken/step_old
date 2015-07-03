function log(str) {
  document.getElementById("result").innerHTML += str + "<br>";
}

function matrix_multiply(n) {
  var a = new Float64Array(n * n);
  var b = new Float64Array(n * n);
  var c = new Float64Array(n * n);
  for (var i = 0; i < n; i++) {
    for (var j = 0; j < n; j++) {
      a[i * n + j] = i * n + j;
      b[i * n + j] = j * n + i;
      c[i * n + j] = 0;
    }
  }

  var begin = performance.now();

  n = n | 0;
  for (var i = 0; i < n; i = (i + 1) | 0) {
    for (var j = 0; j < n; j = (j + 1) | 0) {
      for (var k = 0; k < n; k = (k + 1) | 0) {
        c[i * n + j] += a[i * n + k] * b[k * n + j];
      }
    }
  }

  var end = performance.now();
  log("time: " + ((end - begin) / 1000).toFixed(2));

  var sum = 0;
  for (var i = 0; i < n; i++) {
    for (var j = 0; j < n; j++) {
      sum += c[i * n + j];
    }
  }
  log("sum: " + sum.toFixed(6));
}
