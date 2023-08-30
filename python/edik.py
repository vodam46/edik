#!/bin/env python
import os
import sys
import curses
import string
import traceback


class Editor():
    winx: int
    winy: int
    posy: int = 0
    posx: int = 0
    rows = [""]
    filename: str

    def __init__(self, stdscr, filename=""):
        self.filename = filename
        self.winy, self.winx = stdscr.getmaxyx()
        if filename != "" and os.path.isfile(filename):
            self.rows = open(filename, "r+").read().split("\n")


def draw(stdscr, editor):
    stdscr.clear()
    for y in range(min(editor.winy, len(editor.rows))):
        stdscr.addstr(y, 0, editor.rows[y])
    stdscr.move(editor.posy, editor.posx)


def user_dialogue(stdscr, editor, text):
    stdscr.addstr(editor.winy-1, 0, text)
    ret = ""
    ch = stdscr.getch()
    while ch != 10:
        if chr(ch) in string.printable:
            stdscr.addch(ch)
            ret += chr(ch)
        ch = stdscr.getch()
    return ret


def main(stdscr):
    curses.raw()
    curses.cbreak()
    curses.noecho()
    stdscr.keypad(True)
    editor = Editor(stdscr, sys.argv[1] if len(sys.argv) > 1 else "")
    ch = 0
    search_i = 0
    search_arr = []
    while True:
        draw(stdscr, editor)
        ch = stdscr.getch()
        match ch:
            case 6:  # ^f
                search = user_dialogue(stdscr, editor, "search: ")
                for posy, line in enumerate(editor.rows):
                    # somehow every string (even matching) is -1
                    posx = line.find(search)
                    if posx >= 0:
                        search_arr.append((posy, posx))
                if len(search_arr) == 0:
                    stdscr.addstr(editor.winy-1, 0, f"no {search} found")
            case 14:  # ^n
                if len(search_arr) > 0:
                    editor.posy, editor.posx = search_arr[search_i]
                    search_i = (search_i+1) % len(search_arr)
            case 2:  # ^b
                if len(search_arr) > 0:
                    editor.posy, editor.posx = search_arr[search_i]
                    search_i = (search_i-1) % len(search_arr)

            case 17:  # ^q
                break
            case 19:  # ^s
                if editor.filename == "":
                    editor.filename = user_dialogue(
                        stdscr,
                        editor,
                        "filename: "
                    )
                with open(editor.filename, "w+") as f:
                    f.write("\n".join(editor.rows))

            # arrow keys
            case curses.KEY_DOWN:
                if editor.posy < len(editor.rows):
                    editor.posy += 1
            case curses.KEY_UP:
                if editor.posy > 0:
                    editor.posy -= 1
            case curses.KEY_RIGHT:
                if editor.posx < len(editor.rows[editor.posy]):
                    editor.posx += 1
            case curses.KEY_LEFT:
                if editor.posx > 0:
                    editor.posx -= 1

            case 10:  # enter key
                left = editor.rows[editor.posy][:editor.posx]
                right = editor.rows[editor.posy][editor.posx:]
                editor.rows[editor.posy] = left
                editor.rows.insert(editor.posy+1, right)
                editor.posy += 1
                editor.posx = 0

            case ch if chr(ch) in string.printable:
                editor.rows[editor.posy] = (
                    editor.rows[editor.posy][:editor.posx]
                    + chr(ch)
                    + editor.rows[editor.posy][editor.posx:]
                )
                editor.posx += 1

            case curses.KEY_BACKSPACE:
                pass  # TODO
            case curses.KEY_DC:
                pass  # TODO

            case _:
                editor.rows[editor.posy] = (
                    editor.rows[editor.posy][:editor.posx]
                    + str(ch)
                    + editor.rows[editor.posy][editor.posx:]
                )
                editor.posx += 1
                pass
        stdscr.refresh()
    curses.noraw()
    curses.nocbreak()
    curses.echo()
    stdscr.keypad(False)

    stdscr.clear()
    curses.endwin()


if __name__ == "__main__":
    stdscr = curses.initscr()
    try:
        main(stdscr)
    except Exception:
        curses.noraw()
        curses.nocbreak()
        curses.echo()
        stdscr.keypad(False)

        stdscr.clear()
        curses.endwin()

        traceback.print_exc()
