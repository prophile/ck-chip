<?php

error_reporting(E_ALL | E_STRICT);

$input = file($argv[1]);

$cmds = array();

foreach ($input as $line)
{
	$line = preg_replace('/#(.*)/', '', $line);
	$line = trim($line);
	if ($line != '')
		$cmds[] = $line;
}

$labels = array();

$ip = 0;


foreach ($cmds as $command)
{
	// check if it's a label
	if (substr($command, -1) == ':')
	{
		$labels[substr($command, 0, -1)] = $ip;
	}
	else
	{
		$ip += 2;
	}
}

$output = '';

$opcodes = array('add' => 0, 'addi' => 1, 'and' => 2, 'or' => 3, 'xor' => 4, 'not' => 5, 'rot' => 6, 'str' => 7, 'ldr' => 8, 'stm' => 9, 'ldm' => 10, 'li' => 11, 'jmp' => 12, 'jz' => 13, 'jo' => 14, 
'co' => 15);

$ip = 0;
$line = 0;

function interpret_operand ( $operand )
{
	global $labels;
	global $line;
	if (isset($labels[$operand]))
	{
		return $labels[$operand];
	}
	else
	{
		if ($operand[0] == '$')
		{
			return (int)substr($operand, 1);
		}
		else
		{
			echo "Error: could not interpret operand $operand on line $line\n";
		}
	}
}

foreach ($cmds as $command)
{
	$line++;
	if (substr($command, -1) == ':')
	{
		continue; // ignore labels
	}
	$parts = explode(' ', $command);
	if (count($parts) > 2)
	{
		echo "Error: multi-operand instruction on line $line\n";
		die;
	}
	else
	{
		$op = $parts[0];
		$operand = isset($parts[1]) ? $parts[1] : '$0';
		if (!isset($opcodes[$op]))
		{
			var_dump($parts);
			echo "Error: unknown opcode '$op' on line $line\n";
			die;
		}
		$operand = interpret_operand($operand);
		$outcode = ($opcodes[$op] << 12) | ($operand & 0xFFF);
		$output .= pack('n', $outcode);
	}
	$ip += 2;
}

$fp = fopen($argv[2], 'w');
fwrite($fp, $output);
fclose($fp);
