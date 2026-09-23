$here = $PSScriptRoot
Set-Location $here

function W([string]$name, [string]$content) {
	Set-Content -Path $name -Value $content -NoNewline -Encoding ASCII
}

# -------- 01 hello ---------
W "01_hello.knt" @'
print "Hello, Kinetra!";
'@
W "01_hello.expected" @'
[Kinetra] Hello, Kinetra!
'@

# -------- 02 arithmetic + folding --------
W "02_arith.knt" @'
print 1.0 + 2.0 * 3.0;
print 10.0 / 4.0;
print 0.0 - 5.0;
'@
W "02_arith.expected" @'
[Kinetra] 7
[Kinetra] 2.5
[Kinetra] -5
'@

# --------- 03 variables ----------
W "03_vars.knt" @'
let x = 2.0;
x = x * 3.0;
print x;
'@
W "03_vars.expected" @'
[Kinetra] 6
'@

# --------- 04 control flow ---------
W "04_control.knt" @'
let x = 5.0;
if x < 0.0 {
	print 1.0;
} else if x == 5.0 {
	print 2.0;
} else {
	print 3.0;
}
'@
W "04_control_expected" @'
[Kinetra] 2
'@

# --------- 05 while / break / continue ----------
W "05_while.knt" @'
let i = 0.0;
let out = 0.0;
while i < 5.0 {
	i = i + 1.0;
	if i == 2.0 {
		continue;
	}
	if i == 4.0 {
		break;
	}
	out = out + i;
}
print out;
'@
W "05_while.expected" @'
[Kinetra] 4
'@

# -------- 06 functions + recursion ---------
W "06_functions.knt" @'
fn fib(n) {
	if n < 2.0 {
		return n;
	}
	return fib(n - 1.0) + fib(n - 2.0);
}
print fib(10.0);
'@
W "06_functions.expected" @'
[Kinetra] 55
'@

# -------- 07 lexical scoping --------
W "07_scope.knt" @'
let x = 1.0;
fn probe() {
	let x = 2.0;
	if true {
		let x = 3.0;
		print x;
	}
	print x;
}
probe();
print x;
'@
W "07_scope.expected" @'
[Kinetra] 3
[Kinetra] 2
[Kinetra] 1
'@

# -------- 08 const (valid use) --------
W "08_const.knt" @'
const limit = 10.0;
print limit;
print limit == 10.0;
'@
W "08_const.expected" @'
[Kinetra] 10
[Kinetra] true
'@

# -------- 09 const violation (runtime error) --------
W "09_const_bad.knt" @'
const limit = 1.0;
limit = 2.0;
'@
W "09_const_bad.expected" @'
[Kinetra Error] (runtime) Cannot assign to constant 'limit'
	--> tests/09_const_bad.knt:2
		2 | limit = 2.0;
'@
W "09_const_bad.code" "3"

# -------- 10 vec3 --------
W "10_vec3.knt" @'
let a = vec3(1.0, 2.0, 3.0);
let b = vec3(4.0, 5.0, 6.0);
print dot(a, b);
print cross(a, b);
print normalize(vec3(0.0, 3.0, 4.0));
'@
W "10_vec3.expected" @'
[Kinetra] 32
[Kinetra] vec3(-3, 6, -3)
[Kinetra] vec3(0, 0.6, 0.8)
'@

# -------- 11 mat4 --------
W "11_mat4.knt" @'
let m = translate(1.0, 2.0, 3.0) * scale(2.0, 2.0, 2.0);
print transform(m, vec3(1.0, 1.0, 1.0));
'@
W "11_mat4.expected" @'
[Kinetra] vec3(3, 4, 5)
'@

# -------- 12 arrays --------
W "12_arrays.knt" @'
let a = [1.0, 2.0, 3.0];
a[1] = 20.0;
print len(a);
print a[1];
print [[1.0, 2.0], [3.0]][1][0];
'@
W "12_arrays.expected" @'
[Kinetra] 3
[Kinetra] 20
[Kinetra] 3
'@

# -------- 13 particles + sim --------
W "13_particles.knt" @'
let g = vec3(0.0, 0.0 - 10.0, 0.0);
let p = particle(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), 2.0);
sim 2 dt 0.5 {
	p = apply_force(p, g * p.mass);
	p = integrate(p, dt);
}
print p.position;
print p.velocity;
'@
W "13_particles.expected" @'
[Kinetra] vec3(1, -7.5, 0)
[Kinetra] vec3(1, -10, 0)
'@

# -------- 14 strings --------
W "14_strings.knt" @'
let a = "Hello";
let b = "World";
print a + ", " + b;
print len(b);
print a == "Hello";
'@
W "14_strings.expected" @'
[Kinetra] Hello, World
[Kinetra] 5
[Kinetra] true
'@

# -------- 15 sim forms --------
W "15_sim.knt" @'
let s = 0.0;
sim 4 dt 0.25 {
	s = s + dt;
}
print s;
let t = 0.0;
sim {
	step 3;
	t = t + step_index;
}
print t;
'@
W "15_sim.expected" @'
[Kinetra] 1
[Kinetra] 3
'@

# -------- 16 parallel for --------
W "16_parallel.knt" @'
fn sq(n) {
	return n * n;
}
let out = [0.0, 0.0, 0.0, 0.0];
parallel for i in len(out) {
	out[i] = sq(i);
}
print out[0];
print out[3];
'@
W "16_parallel.expected" @'
[Kinetra] 0
[Kinetra] 9
'@

# -------- 17 short-circuit logic --------
W "17_logic.knt" @'
fn side() {
	print 99.0;
	return true;
}
print false && side();
print true || side();
print !true;
'@
W "17_logic.expected" @'
[Kinetra] false
[Kinetra] true
[Kinetra] false
'@

# -------- 18 lex error --------
W "18_lex_bad.knt" @'
let s = "oops;
'@
W "18_lex_bad.expected" @'
[Kinetra Error] (lex) Unterminated string literal
	--> tests/18_lex_bad.knt:1:9
		1 | let s = "oops;
		|        ^
'@
W "18_let_bad.code" "1"

# -------- 19 parse error --------
W "19_parse_bad.knt" @'
let x (1 + 2;
'@
W "19_parse_bad.expected" @'
[Kinetra Error] (parse) Expected ')'
	--> tests/19_parse_bad.knt:1:15
		1: let x = (1 + 2;
		|           ^ 
'@
W "19_parse_bad.code" "2"

# -------- 20 runtime error --------
W "20_runtime_bad.knt" @'
print 1.0 / 0.0;
'@
W "20_runtime_bad.expected" @'
[Kinetra Error] (runtime) Division by zero
	--> tests/20_runtime_bad.knt:1
		1 | print 1.0 / 0.0;
'@
W "20_runtime_bad.code" "3"

Write-Host "Generated 20 test cases in $here"