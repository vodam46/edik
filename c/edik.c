#include <wchar.h>
#include <locale.h>
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

typedef struct {
	char** lines;
	char* filename;
	vector pos;
	vector scr_size;
	int line_num;
	int scroll_offset;
} buffer;

void end() {
	nocbreak();
	echo();
	nl();
	noraw();
	keypad(stdscr, FALSE);
	endwin();
}
void start(vector* scr_size, WINDOW** main_scr, WINDOW** message_scr) {
	initscr();
	cbreak();
	noecho();
	nonl();
	raw();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, scr_size->y, scr_size->x);
	(*main_scr) = newwin(scr_size->y-1, scr_size->x, 0, 0);
	keypad((*main_scr), TRUE);
	(*message_scr) = newwin(1, scr_size->x, scr_size->y-1, 0);
	keypad((*message_scr), TRUE);
}

char* add_to_line(char* old_line, char ch, int x) {
	char* line = malloc(strlen(old_line));
	strncpy(line, old_line, x);
	line[x]=ch;
	line[x+1]=0;
	strcat(line, &old_line[x]);
	old_line = malloc(strlen(line));
	strcpy(old_line, line);
	old_line[strlen(line)] = 0;
	free(line);
	return old_line;
}

char* remove_char(char* line, int x) {
	for (int i = x-1; i < (int)strlen(line); i++) {
		line[i] = line[i+1];
	}
	line = realloc(line, strlen(line));
	return line;
}

void draw(buffer edik, WINDOW* main_scr) {
	wclear(main_scr);
	for (
			int scr_y = 0, lines_y = edik.scroll_offset;
			lines_y <= edik.scr_size.y+edik.scroll_offset;
			scr_y++, lines_y++
		) {
		if (lines_y < edik.line_num) {
			for (
					int scr_x = 0, lines_x = 0;
					lines_x < (int)strlen(edik.lines[lines_y]);
					scr_x++, lines_x++
				) {
				if (edik.lines[lines_y][lines_x] != '\t')
					mvwaddstr(main_scr, scr_y, 0, edik.lines[lines_y]);
				else
					scr_x+=3;
			}
		}
	}
	wrefresh(main_scr);
}

void save(buffer edik) {
	FILE* file = fopen(edik.filename, "w");
	file = freopen(edik.filename, "a", file);
	endwin();
	for (int i = 0; i < edik.line_num; i++) {
		fwrite(edik.lines[i], 1, strlen(edik.lines[i]), file);
	}
	fclose(file);
}

vector find_word(buffer edik, char* query, int offset) {
	vector new_pos;
	new_pos = (vector){edik.pos.x+offset, edik.pos.y};
	if (offset == -1) {
		if (new_pos.x < 0) {
			new_pos.y--;
			if (new_pos.y >= 0)
				new_pos.x = strlen(edik.lines[edik.pos.y])-1;
		}
	}
	for (int y = new_pos.y; y  < edik.line_num && y >= 0; y += offset) {
		for (int i = new_pos.x;
				i < (int)strlen(edik.lines[y]) && i >= 0;
				i += offset) {
			int loop = 1;
			for (int j = 0; j  < (int)strlen(query); j++) {
				if (edik.lines[y][i+j] != query[j]) {
					loop = 0;
					new_pos.y += offset;
					if (offset == 1)
						new_pos.x = 0;
					else
						if (new_pos.y >= 0)
							new_pos.x = strlen(edik.lines[new_pos.y])-1;
					break;
				}
			}
			if (loop) {
				if (
						i >= 0 && i <= (int)strlen(edik.lines[y])
						&& y >= 0 && y < edik.line_num
				   ) return (vector){i, y};
			}
		}
	}
	return edik.pos;
}

void send_message(WINDOW* message_scr, char* fmt, ...) {
	va_list args;
	wclear(message_scr);
	va_start(args, fmt);
	wmove(message_scr, 0, 0);
	vw_printw(message_scr, fmt, args);
	va_end(args);
	wrefresh(message_scr);
}

char* get_user_input(char* prompt, WINDOW* message_scr) {
	send_message(message_scr, "%s", prompt);
	char* ret_str = malloc(sizeof(char));
	ret_str[0] = 0;
	int* ch = malloc(sizeof(int));
	int i = 0;
	while (1) {
		ch[0] = wgetch(message_scr);
		switch (ch[0]) {
			case '\r':
			case '\n':
			case KEY_BREAK:
				return ret_str;
			case 27:
			case CTRL('c'):
			case CTRL('q'):
				return "";
			case KEY_BACKSPACE:
				if (i != 0) {
					ret_str = remove_char(ret_str, i);
					i--;
				}
				if (i == 0) {
					ret_str = "";
				}
				break;
			default:
				ret_str = add_to_line(ret_str, ch[0], i);
				i++;
		}
		send_message(message_scr, "%s%s", prompt, ret_str);
	}
}

int main(int argc, char** argv) {
	setlocale(LC_ALL, "");
	buffer edik = {0};
	WINDOW* main_scr = {0};
	WINDOW* message_scr = {0};
	start(&edik.scr_size, &main_scr, &message_scr);
	atexit(end);

	FILE* file;
	if (argc == 1) {
		edik.filename = get_user_input("Choose filename: ", message_scr);
		if (strlen(edik.filename) == 0) {
			end();
			printf("Empty filenames are not possible\n");
			return 1;
		}
	} else if (argc == 2) {
		edik.filename = argv[1];

	} else {
		end();
		fprintf(stderr, "Wrong arguemnts\n");
		return 1;
	}
	file = fopen(edik.filename, "r");
	if (file == NULL) {
		fclose(fopen(edik.filename, "a+"));
		file = fopen(edik.filename, "r");
	}

	while (!feof(file)) {
		int ch = fgetc(file);
		if (ch == '\n') {
			edik.line_num++;
		}
	}
	unsigned int fsize = ftell(file);
	rewind(file);
	if (fsize != 0) {
		edik.lines = malloc(sizeof(char*) * edik.line_num);
		size_t len;
		for (int i = 0; i < edik.line_num; i++) {
			getline(&edik.lines[i], &len, file);
		}
	} else {
		edik.line_num++;
		edik.lines = malloc(sizeof(char*) * edik.line_num);
		edik.lines[0] = "\n\0";
	}
	fclose(file);

	edik.pos = (vector){0,0};
	short int loop = 1;

	char* search_str;
	char* command_string;
	while (loop) {
		draw(edik, main_scr);
		send_message(message_scr, "%s", edik.filename);
		int tab_offset = 0;
		for (int i = 0; i < edik.pos.x; i++) {
			if (edik.lines[edik.pos.y][i] == '\t') {
				tab_offset += 3;
			}
		}
		wmove(main_scr, edik.pos.y - edik.scroll_offset, edik.pos.x + tab_offset);
		int ch = wgetch(main_scr);
		switch (ch) {
			case KEY_RESIZE:
				getmaxyx(stdscr, edik.scr_size.y, edik.scr_size.x);
				break;

			case CTRL('q'):
				loop=0;
				break;
			case CTRL('s'):
				save(edik);
				break;

			case CTRL('f'):
				search_str = get_user_input("Search: ", message_scr);
				break;
			case CTRL('n'):
				edik.pos = find_word(edik, search_str, 1);
				break;
			case CTRL('p'):
				edik.pos = find_word(edik, search_str, -1);
				break;

			case CTRL('x'):
				command_string = get_user_input("Command: ", message_scr);
				if (strlen(command_string)) {
					end();
					printf("%s\n", command_string);
					printf("\n%d\n\nPress Enter to continue", system(command_string));
					getchar();
					start(&edik.scr_size, &main_scr, &message_scr);
				}
				break;

			case KEY_LEFT:
				edik.pos.x = MAX(0, edik.pos.x - 1);
				break;
			case KEY_RIGHT:
				edik.pos.x = MIN((int)strlen(edik.lines[edik.pos.y])-1, edik.pos.x+1);
				break;
			case KEY_DOWN:
				edik.pos.y = MIN(edik.line_num-1, edik.pos.y+1);
				edik.pos.x = MIN((int)strlen(edik.lines[edik.pos.y])-1, edik.pos.x);
				break;
			case KEY_UP:
				edik.pos.y = MAX(0, edik.pos.y - 1);
				edik.pos.x = MIN((int)strlen(edik.lines[edik.pos.y])-1, edik.pos.x);
				break;

			case KEY_BREAK:
			case '\n':
			case '\r':
				edik.line_num++;
				char* left = malloc((edik.pos.x+1));
				strncpy(left, edik.lines[edik.pos.y], edik.pos.x);
				left[edik.pos.x] = '\n';
				left[edik.pos.x+1] = 0;
				char* right;
				if (edik.pos.x != (int)strlen(edik.lines[edik.pos.y])-1) {
					right = malloc((strlen(edik.lines[edik.pos.y])-edik.pos.x));
					strcpy(right, &edik.lines[edik.pos.y][edik.pos.x]);
				} else {
					right = "\n";
				}
				edik.lines = realloc(edik.lines, edik.line_num * sizeof(char*));
				edik.lines[edik.pos.y] = left;
				for (int i = edik.line_num-1; i > edik.pos.y; i--) {
					edik.lines[i] = edik.lines[i-1];
				}
				edik.lines[edik.pos.y+1] = right;
				edik.pos.x = 0;
				edik.pos.y += 1;
				break;

			case KEY_BACKSPACE:
				if (edik.pos.x != 0) {
					edik.lines[edik.pos.y] = remove_char(
						edik.lines[edik.pos.y],
						edik.pos.x
					);
					edik.pos.x--;
				} else if (edik.pos.y != 0) {
					int new_x = strlen(edik.lines[edik.pos.y-1]);
					char* new_line = malloc(
						(new_x+strlen(edik.lines[edik.pos.y])+1)
					);
					new_line[0] = 0;
					strcat(new_line, edik.lines[edik.pos.y-1]);
					new_line[strlen(new_line)-1] = 0;
					strcat(new_line, edik.lines[edik.pos.y]);
					for (int i = edik.pos.y; i < edik.line_num; i++) {
						edik.lines[i] = edik.lines[i+1];
					}
					edik.lines[edik.pos.y-1] = new_line;
					edik.line_num--;
					edik.lines = realloc(edik.lines, edik.line_num);
					edik.pos.y--;
					edik.pos.x = new_x-1;
				}
				break;

			default:
				if (ch >= 32) {
					edik.lines[edik.pos.y] = add_to_line(
						edik.lines[edik.pos.y],
						ch,
						edik.pos.x
					);
					edik.pos.x++;
				}
				break;
		}
		while (edik.pos.y - edik.scroll_offset >= edik.scr_size.y-1) {
			edik.scroll_offset++;
		}
		while (edik.pos.y - edik.scroll_offset < 0) {
			edik.scroll_offset = MAX(edik.scroll_offset-1, 0);
		}
	}
	end();
	for (int i = 0; i < edik.line_num; i++) {
		if (strlen(edik.lines[i]) != 1) {
			free(edik.lines[i]);
		}
	}
	free(edik.lines);
	return 0;
}
