#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#ifndef CTRL
#define CTRL(c) ((c)&0x1f)
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef struct {
	int x, y;
} vector;

void end() {
	nocbreak();
	echo();
	nl();
	noraw();
	keypad(stdscr, FALSE);
	endwin();
}
void start() {
	initscr();
	cbreak();
	noecho();
	nonl();
	raw();
	keypad(stdscr, TRUE);
}

void draw(char** lines, int scroll_offset, int line_num, vector scr_size) {
	clear();
	for (int y = 0, i = scroll_offset; i <= scr_size.y+scroll_offset; y++, i++) {
		if (i < line_num) {
			for (int x = 0, j = 0; j < strlen(lines[i]); x++, j++) {
				if (lines[i][j] != '\t')
					mvaddch(y, x, lines[i][j]);
				else
					x+=3;
			}
		}
	}
	refresh();
}

char** add_to_lines(char** lines, char ch, vector pos) {
	char* line = malloc(strlen(lines[pos.y])+sizeof(ch));
	strncpy(line, lines[pos.y], pos.x);
	line[pos.x]=ch;
	line[pos.x+1]=0;
	strcat(line, &lines[pos.y][pos.x]);
	lines[pos.y] = malloc(strlen(line)+sizeof(char));
	strcpy(lines[pos.y], line);
	lines[pos.y][strlen(line)] = 0;
	free(line);
	return lines;
}

void save(char* filename, char** lines, int line_num) {
	FILE* file = fopen(filename, "w");
	file = freopen(filename, "a", file);
	endwin();
	for (int i = 0; i < line_num; i++) {
		fwrite(lines[i], 1, strlen(lines[i]), file);
	}
	fclose(file);
}

vector find_next(char** lines, char* query, vector pos, int line_num) {
	for (int i = pos.y; i < line_num; i++) {
		char* needle = strstr(&lines[i][pos.x+1], query);
		if (needle !=NULL) {
			int new_x = needle-lines[i];
			if (new_x != pos.x)
				return (vector){new_x, i};
		}
	}
	return pos;
}
vector find_prev(char** lines, char* query, vector pos, int line_num) {
	for (int i = pos.y; i > 0; i--) {
		char* needle = strstr(lines[i], query);
		if (needle !=NULL) {
			return (vector){needle-lines[i], i};
		}
	}
	return pos;
}

char* get_user_input(char* prompt, vector scr_size) {
	move(scr_size.y-1, 0);
	refresh();
	printw("%s", prompt);
	char* ret_str = malloc(sizeof(char));
	ret_str[0] = 0;
	int* ch = malloc(sizeof(int));
	ch[0] = getch();
	while (ch[0] != '\r' && ch[0] != '\n' && ch[0] != KEY_BREAK) {
		addch(ch[0]);
		strcat(ret_str, (char*)ch);
		ret_str = realloc(ret_str, strlen(ret_str)+sizeof(char*));
		ret_str[sizeof(char*)] = 0;
		ch[0] = getch();
	}
	return ret_str;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("wrong arguments, creating new files is not implemented yet\n");
		return 1;
	}
	char* filename = argv[1];
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("error when reading file\n");
		return 1;
	}

	int line_num = 0;
	while (!feof(file)) {
		int ch = fgetc(file);
		if (ch == '\n') {
			line_num++;
		}
	}
	unsigned int fsize = ftell(file);
	rewind(file);
	char** lines;
	if (fsize != 0) {
		lines = malloc(sizeof(char*) * line_num);
		size_t len;
		for (int i = 0; i < line_num; i++) {
			getline(&lines[i], &len, file);
		}
	} else {
		line_num++;
		lines = malloc(sizeof(char*) * line_num);
		lines[0] = "\n\0";
	}
	fclose(file);

	start();
	atexit(end);

	vector scr_size;
	getmaxyx(stdscr, scr_size.y, scr_size.x);

	vector pos = {0,0};
	int loop = 1;

	char* search_str;
	char* command_string;
	int scroll_offset = 0;
	while (loop) {
		draw(lines, scroll_offset, line_num, scr_size);
		int tab_offset = 0;
		for (int i = 0; i < pos.x; i++) {
			if (lines[pos.y][i] == '\t') {
				tab_offset += 3;
			}
		}
		move(pos.y - scroll_offset, pos.x + tab_offset);
		int ch = getch();
		switch (ch) {

			case KEY_RESIZE:
				getmaxyx(stdscr, scr_size.y, scr_size.x);
				break;

			case CTRL('q'):
				loop=0;
				break;
			case CTRL('s'):
				save(filename, lines, line_num);
				break;

			case CTRL('f'):
				search_str = get_user_input("Search: ", scr_size);
				break;
			case CTRL('n'):
				pos = find_next(lines, search_str, pos, line_num);
				break;
			case CTRL('p'):
				pos = find_prev(lines, search_str, pos, line_num);
				break;

			case CTRL('x'):
				command_string = get_user_input("Command: ", scr_size);
				end();
				printf("%s\n", command_string);
				system(command_string);
				printf("\n\nPress any key to continue");
				getchar();
				start();
				break;

			case KEY_LEFT:
				pos.x = MAX(0, pos.x - 1);
				break;
			case KEY_RIGHT:
				pos.x = MIN(strlen(lines[pos.y])-1, pos.x+1);
				break;
			case KEY_DOWN:
				pos.y = MIN(line_num-1, pos.y+1);
				pos.x = MIN(strlen(lines[pos.y])-1, pos.x);
				if (pos.y - scroll_offset >= scr_size.y) {
					scroll_offset++;
				}
				break;
			case KEY_UP:
				if (pos.y - scroll_offset <= 0) {
					scroll_offset = MAX(scroll_offset-1, 0);
				}
				pos.y = MAX(0, pos.y - 1);
				pos.x = MIN(strlen(lines[pos.y])-1, pos.x);
				break;

			case KEY_BREAK:
			case '\n':
			case '\r':
				line_num++;
				char* left = malloc((pos.x+1)*sizeof(char));
				strncpy(left, lines[pos.y], pos.x);
				left[pos.x] = '\n';
				left[pos.x+1] = 0;
				char* right;
				if (pos.x != strlen(lines[pos.y])-1) {
					right = malloc((strlen(lines[pos.y]) - pos.x)*sizeof(char));
					strcat(right, &lines[pos.y][pos.x]);
				} else {
					right = "\n";
				}
				lines = realloc(lines, line_num * sizeof(char*));
				lines[pos.y] = left;
				for (int i = line_num-1; i > pos.y; i--) {
					lines[i] = lines[i-1];
				}
				lines[pos.y+1] = right;
				pos.x = 0;
				pos.y += 1;
				break;

			case KEY_BACKSPACE:
				if (pos.x != 0) {
					for (int i = pos.x-1; i < strlen(lines[pos.y]); i++) {
						lines[pos.y][i] = lines[pos.y][i+1];
					}
					lines[pos.y] = realloc(lines[pos.y], strlen(lines[pos.y]));
					pos.x--;
				} else if (pos.y != 0) {
					int new_x = strlen(lines[pos.y-1]);
					char* new_line = malloc(
							(new_x+strlen(lines[pos.y])+1)*sizeof(char)
							);
					new_line[0] = 0;
					strcat(new_line, lines[pos.y-1]);
					new_line[strlen(new_line)-1] = 0;
					strcat(new_line, lines[pos.y]);
					for (int i = pos.y; i < line_num; i++) {
						lines[i] = lines[i+1];
					}
					lines[pos.y-1] = new_line;
					line_num--;
					lines = realloc(lines, line_num*sizeof(char*));
					pos.y--;
					pos.x = new_x-1;
				}
				break;

			default:
				if (ch >= 32) {
					lines = add_to_lines(lines, ch, pos);
					pos.x++;
				}
				break;
		}
	}
	end();
	free(lines);
}
