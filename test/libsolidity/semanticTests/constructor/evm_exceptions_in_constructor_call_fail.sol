contract A {
    constructor() {
        address(this).call("123");
    }
}


contract B {
    uint256 public test = 1;

    function testIt() public {
        A a = new A();
        ++test;
    }
}
// ====
// bytecodeFormat: legacy,>=EOFv1
// targetContract: B
// ----
// testIt() ->
// test() -> 2
