time printf 'Dit is een test\nFoo bar baz\nend\n' | cat bf/woorden.bf programs/nul - \
	| ./interpreter -* programs/brainfuck.txt
