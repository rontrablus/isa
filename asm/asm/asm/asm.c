#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//define constants as given in the instructions
#define MEMORY_SIZE 4096
#define MAX_LINE_LENGTH 500 
#define MAX_LABEL_LENGTH 50

typedef char line[MAX_LINE_LENGTH + 1];
typedef struct {
	char name[MAX_LABEL_LENGTH + 1];
	int location;
} label;

typedef struct {
	char *label;
	char *opcode;
	char *rd;
	char *rs;
	char *rt;
	char *imm;
} parsed_instruction;

typedef enum {
	EMPTY,
	LABEL,
	LABEL_REGULAR, //label & regular instruction in one line
	WORD,
	REGULAR, //opcode, rd, rs, rt, imm
} line_type;

//declaration of functions
parsed_instruction parse_line(char *line);
line_type find_line_type(parsed_instruction ins);
void assemble(FILE *input_file, FILE *output_file);
void first_pass(FILE *input_file);
int second_pass(FILE *input_file);
int opcode_to_number(char *opcode);
int register_to_number(char *reg);
int immediate_to_number(char *imm);
int encode_instruction(int opcode, int rd, int rs, int rt, int imm);
int HexToInt2sComp(char * h);
int HexCharToInt(char h);

label labels[MEMORY_SIZE]; //global array to store the labels
int labels_amount = 0;
int memory[MEMORY_SIZE]; //global array to store the memory

int main(int argc, char *argv[]) {
	FILE *program = fopen(argv[1], "r"), *output = fopen(argv[2], "w"); // *output = NULL;
	if (program == NULL || output == NULL) {
		printf("Failed to open one of the files"); //Failed to open one of the files
		exit(1);
	}
	assemble(program, output); //read from the input file and write to the output file
	exit(0);
}

// parses the line into: opcode, rd, rs, rt, imm
parsed_instruction parse_line(char *line) {
	strtok(line, "#"); // To get rid of comments
	char *line_end = line + strlen(line);
	char *current = line;
	parsed_instruction result = { 0 };
	char *x = strtok(current, ": \t\n,");
	if (current >= line_end) {
		return result;
	}
	if ((x != NULL) && opcode_to_number(x) != -1)//if not label
	{
		result.label = NULL;
		result.opcode = x;
	}
	else //if label
	{
		result.label = x;
		current = current + strlen(current) + 1;
		if (current >= line_end)
			return result;
		result.opcode = strtok(current, ": \t\n,");
	}

	//in the following segment we check everytime if the line ended. if not we continue to parse the rest of the line

	current = current + strlen(current) + 1;
	if (current >= line_end)
		return result;

	result.rd = strtok(current, " \t\n,");
	current = current + strlen(current) + 1;
	if (current >= line_end)
		return result;

	result.rs = strtok(current, " \t\n,");
	current = current + strlen(current) + 1;
	if (current >= line_end)
		return result;

	result.rt = strtok(current, " \t\n,");
	current = current + strlen(current) + 1;
	if (current >= line_end)
		return result;

	result.imm = strtok(current, " \t\n,");
	return result;
}

// this function determines what kind of line it is.
line_type find_line_type(parsed_instruction ins) {
	if ((ins.label != NULL && ins.label[0] == '#') || (ins.label == NULL && ins.opcode == NULL))
		return EMPTY;
	if (ins.label != NULL && ins.opcode == NULL)
		return LABEL;
	if (ins.label != NULL && ins.opcode != NULL)
		return LABEL_REGULAR;
	if (strcmp(ins.opcode, ".word") == 0)
		return WORD;
	if (ins.label == NULL && ins.opcode != NULL)
		return REGULAR;
}

//go over the file for the first time and insert label to the array 'labels' 
void first_pass(FILE *input_file) {
	int current_memory_location = 0;
	line l = { 0 };
	while (fgets(l, MAX_LINE_LENGTH, input_file) != NULL) {
		parsed_instruction ins = parse_line(l); //split the line into ins
		line_type lt = find_line_type(ins);
		if (lt == LABEL || lt == LABEL_REGULAR) //if the line type is label insert the label to the array labels
		{
			strcpy(labels[labels_amount].name, ins.label);
			labels[labels_amount].location = current_memory_location;
			labels_amount++;
		}
		if (lt != EMPTY && lt != WORD && lt != LABEL)
			current_memory_location++;
		if ((lt == REGULAR) || (lt == LABEL_REGULAR))
		{
			int opcode = opcode_to_number(ins.opcode);
			/*if (opcode <= 13 && opcode >= 7) //check if instruction has immediate
			{
				if (opcode == 8) //check if instruction is not branch jr
				{
					int type_of_branch = branch_to_number(ins.rd);
					if (type_of_branch != 6)
						current_memory_location++;
				}
				else
					current_memory_location++;
			}*/
		}
	}
}

//go over the file for the second time
int second_pass(FILE *input_file) {
	fseek(input_file, 0, SEEK_SET);
	int current_memory_location = 0;
	int program_length = 0;
	line l = { 0 };

	while (fgets(l, MAX_LINE_LENGTH, input_file) != NULL) {
		parsed_instruction ins = parse_line(l);
		line_type lt = find_line_type(ins);
		if (lt == WORD) {
			int address;
			int data;
			if (ins.rd[0] == '0' && (ins.rd[1] == 'x' || ins.rd[1] == 'X')) //check if address is hex 
				address = HexToInt2sComp(ins.rd);
			else
				address = atoi(ins.rd);
			if (ins.rs[0] == '0' && (ins.rs[1] == 'x' || ins.rs[1] == 'X')) //check if data is hex
				data = HexToInt2sComp(ins.rs);
			else
				data = atoi(ins.rs) & 0xffff;
			memory[address] = data;
			if (address >= program_length)
				program_length = address + 1;
		}
		else if ((lt == REGULAR) || (lt == LABEL_REGULAR)) {
			int opcode = opcode_to_number(ins.opcode);
			int rd = register_to_number(ins.rd);
			int rs = register_to_number(ins.rs);
			int rt = register_to_number(ins.rt);
			int imm = immediate_to_number(ins.imm);
			int encoded = encode_instruction(opcode, rd, rs, rt, imm);
			memory[current_memory_location] = encoded;
			current_memory_location++;

			
			if (opcode <= 13 && opcode >= 7) //check if instruction has immediate
			{
				/*if (opcode == 8) //check if instruction is not branch jr 
				{
					int type_of_brance = branch_to_number(ins.rd);
					if (type_of_brance != 6)
					{
						imm = imm & 0xffff;
						memory[current_memory_location] = imm;
						current_memory_location++;
					}
				}
				else {
					imm = imm & 0xffff;
					memory[current_memory_location] = imm;
					current_memory_location++;
				}*/
			}
		}
	}
	if (current_memory_location > program_length)
		program_length = current_memory_location;
	return program_length;
}
// convert opcode to number
int opcode_to_number(char *opcode) {
	char temp[MAX_LINE_LENGTH];
	strcpy(temp, opcode);
	_strlwr(temp); //not case sensitive
	if (!strcmp(temp, "add"))
		return 0;
	else if (!strcmp(temp, "sub"))
		return 1;
	else if (!strcmp(temp, "and"))
		return 2;
	else if (!strcmp(temp, "or"))
		return 3;
	else if (!strcmp(temp, "sll"))
		return 4;
	else if (!strcmp(temp, "sra"))
		return 5;
	else if (!strcmp(temp, "srl"))
		return 6;
	else if (!strcmp(temp, "beq"))
		return 7;
	else if (!strcmp(temp, "bne"))
		return 8;
	else if (!strcmp(temp, "blt"))
		return 9;
	else if (!strcmp(temp, "bgt"))
		return 10;
	else if (!strcmp(temp, "ble"))
		return 11;
	else if (!strcmp(temp, "bge"))
		return 12;
	else if (!strcmp(temp, "jal"))
		return 13;
	else if (!strcmp(temp, "lw"))
		return 14;
	else if (!strcmp(temp, "sw"))
		return 15;
	else if (!strcmp(temp, "reti"))
		return 16;
	else if (!strcmp(temp, "in"))
		return 17;
	else if (!strcmp(temp, "out"))
		return 18;
	else if (!strcmp(temp, "halt"))
		return 19;
	return -1;
	try to put it in the git
}

//convert Register name to Register number
int register_to_number(char *reg) {
	char temp[MAX_LINE_LENGTH];
	strcpy(temp, reg);
	_strlwr(temp); //not case sensitive
	if (reg == NULL)
		return 0;
	else if (!strcmp(temp, "$zero"))
		return 0;
	else if (!strcmp(temp, "$imm"))
		return 1;
	else if (!strcmp(temp, "$v0"))
		return 2;
	else if (!strcmp(temp, "$a0"))
		return 3;
	else if (!strcmp(temp, "$a1"))
		return 4;
	else if (!strcmp(temp, "$t0"))
		return 5;
	else if (!strcmp(temp, "$t1"))
		return 6;
	else if (!strcmp(temp, "$t2"))
		return 7;
	else if (!strcmp(temp, "$t3"))
		return 8;
	else if (!strcmp(temp, "$s0"))
		return 9;
	else if (!strcmp(temp, "$s1"))
		return 10;
	else if (!strcmp(temp, "$s2"))
		return 11;
	else if (!strcmp(temp, "$gp"))
		return 12;
	else if (!strcmp(temp, "$sp"))
		return 13;
	else if (!strcmp(temp, "$fp"))
		return 14;
	else if (!strcmp(temp, "$ra"))
		return 15;
	return atoi(temp);
}

// convert char imm to number
int immediate_to_number(char *imm) {
	if (isalpha(imm[0])) //label
	{
		for (int i = 0; i < labels_amount; i++)
		{
			if (strcmp(imm, labels[i].name) == 0)
				return labels[i].location;
		}
	}
	if (imm[0] == '0' && (imm[1] == 'x' || imm[1] == 'X')) //Hex number
	{
		return HexToInt2sComp(imm);
	}
	return atoi(imm); // returns the number
}

// Encodeing instruction by shifting the registers to the right place to get 32bit
int encode_instruction(int opcode, int rd, int rs, int rt, int imm) {
	return ((opcode << 24) + (rd << 20) + (rs << 16) + (rt << 12) + (imm));
}

//convert Hex to int in 2's complement
int HexToInt2sComp(char * h) {
	int i;
	int res = 0;
	int len = strlen(h);
	for (i = 0; i < len; i++)
	{
		res += HexCharToInt(h[len - 1 - i]) * (1 << (4 * i)); // change char by char from right to left, and shift it left 4 times
	}
	if ((len <= 4) && (res & (1 << (len * 4 - 1)))) // if len is less than 4 and the msb is 1, we want to sign extend the number
	{
		res |= -1 * (1 << (len * 4)); // or of the number with: 1 from the 3rd char until the msb of res, then zeros till lsb. -1 for the sign.
	}
	return res;
}

//convert char to int
int HexCharToInt(char h) {
	short res;
	switch (h) {
	case 'A':
		res = 10;
		break;
	case 'B':
		res = 11;
		break;
	case 'C':
		res = 12;
		break;
	case 'D':
		res = 13;
		break;
	case 'E':
		res = 14;
		break;
	case 'F':
		res = 15;
		break;
	case 'a':
		res = 10;
		break;
	case 'b':
		res = 11;
		break;
	case 'c':
		res = 12;
		break;
	case 'd':
		res = 13;
		break;
	case 'e':
		res = 14;
		break;
	case 'f':
		res = 15;
		break;
	default:
		res = atoi(&h); // if char < 10 change it to int
		break;
	}
	return res;
}

// the assebler function - go over the input file twice, and writes into the output file
void assemble(FILE *input_file, FILE *output_file) {
	first_pass(input_file);
	int program_length = second_pass(input_file);
	for (int i = 0; i < program_length; i++) {
		/*fprintf(output_file, "%04X", memory[i]);*/
		fwrite("\n", 1, 1, output_file);// writes \n into the file
	}
}
