"""
Main python script. Connects to c backend and provides GUI, with a resizing
game of life grid
"""

from __future__ import annotations
import ctypes
from enum import IntEnum
from pathlib import Path
from time import time
from typing import TypeAlias
import math
from tkinter import (
    BOTH,
    LEFT,
    RIGHT,
    Y,
    Canvas,
    DoubleVar,
    IntVar,
    StringVar,
    Tk,
    filedialog,
    messagebox,
)
from tkinter import ttk

# states for alive and dead
ALIVE = 1
DEAD = 0

# colours for cells
ALIVE_COL = "#263238"
DEAD_COL = "#fffaf2"

# other colours
BACKGROUND = "#f6f0e6"
BOARD_COL = "#fffaf2"
OUTLINE_COL = "#d6c9b6"


# constants for birth/ survival rates
RECT_DEFAULT_BIRTH = 3
RECT_DEFAULT_SURVIVAL_MIN = 2
RECT_DEFAULT_SURVIVAL_MAX = 3

HEX_DEFAULT_BIRTH = 2
HEX_DEFAULT_SURVIVAL_MIN = 3
HEX_DEFAULT_SURVIVAL_MAX = 4


class GridType(IntEnum):  # enum class
    RECT = 0
    HEX = 1


class Cell(ctypes.Structure):
    # follows structure of c cell struct
    _fields_ = [("status", ctypes.c_ubyte)]

# rule set struct
class RuleSet(ctypes.Structure):
    _fields_ = [
        ("birth_count", ctypes.c_int),
        ("survival_min", ctypes.c_int),
        ("survival_max", ctypes.c_int),
    ]


Cell_Ptr: TypeAlias = ctypes.POINTER(Cell)


class Backend:
    def __init__(self) -> None:
        # need the gol library
        lib_path = Path(__file__).resolve().parent / "build" / "libgol.so"
        if not lib_path.exists():
            raise FileNotFoundError(
                "no libgol.so library found at "
                + f"{lib_path}."
                + " build from extension with "
                + "'make libgol.so'"
            )

        self.lib = ctypes.CDLL(str(lib_path))  # load c file

        cell_ptr = ctypes.POINTER(Cell)  # Cell* for function types below

        # FUNCTION TYPES:

        # for mallocing grid
        self.lib.malloc_grid.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.malloc_grid.restype = cell_ptr

        # for freeing grid
        self.lib.free_grid.argtypes = [cell_ptr]
        self.lib.free_grid.restype = None

        # starting grids
        self.lib.empty_grid.argtypes = [cell_ptr, ctypes.c_int, ctypes.c_int]
        self.lib.empty_grid.restype = None

        self.lib.random_grid.argtypes = [cell_ptr, ctypes.c_int, ctypes.c_int]
        self.lib.random_grid.restype = None

        # random seed so that it doesn't generate the same grid on random call
        self.lib.seed_random.argtypes = [ctypes.c_uint]
        self.lib.seed_random.restype = None

        # update grid
        self.lib.update_grid.argtypes = [
            cell_ptr,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.c_int,
            RuleSet,
        ]
        self.lib.update_grid.restype = None

        # save/export grid types
        self.lib.save_grid.argtypes = [
            ctypes.c_char_p,
            cell_ptr,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
        ]
        self.lib.save_grid.restype = ctypes.c_bool

        # load/import grid types
        self.lib.load_grid.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.load_grid.restype = cell_ptr

        self.lib.seed_random(int(time()))  # the seed is just the time

        # FUNCTIONS, mainly just acting as wrappers around the c functions

    def malloc_grid(self, width: int, height: int) -> Cell_Ptr:
        return self.lib.malloc_grid(width, height)

    def free_grid(self, grid: Cell_Ptr) -> None:
        self.lib.free_grid(grid)

    def empty_grid(self, grid: Cell_Ptr, width: int, height: int) -> None:
        self.lib.empty_grid(grid, width, height)

    def random_grid(self, grid: Cell_Ptr, width: int, height: int) -> None:
        self.lib.random_grid(grid, width, height)

    def update_grid(
        self,
        grid: Cell_Ptr,
        width: int,
        height: int,
        generations: ctypes.c_ulong,
        grid_type: GridType,
        rules: RuleSet,
    ) -> None:
        self.lib.update_grid(
            grid,
            width,
            height,
            ctypes.byref(generations),
            int(grid_type),
            rules,
        )

    def save_grid(
        self,
        path: str,
        grid: Cell_Ptr,
        width: int,
        height: int,
        grid_type: GridType,
    ) -> bool:
        return bool(
            self.lib.save_grid(
                path.encode("utf-8"), grid, width, height, int(grid_type)
            )
        )

    def load_grid(
        self, path: str
    ) -> tuple[Cell_Ptr, int, int, GridType] | None:
        # returns None on fail

        # declare tyeps
        width = ctypes.c_int()
        height = ctypes.c_int()
        grid_type = ctypes.c_int()

        grid = self.lib.load_grid(  # and then give address for declaration
            path.encode("utf-8"),
            ctypes.byref(width),
            ctypes.byref(height),
            ctypes.byref(grid_type),
        )
        if not grid:
            return None
        return grid, width.value, height.value, GridType(grid_type.value)


class GameOfLife:
    # class attrs
    # possible board sizes
    SIZES: dict[str, tuple[int, int]] = {
        "15 x 15": (15, 15),
        "25 x 25": (25, 25),
        "50 x 50": (50, 50),
    }

    DRAWN_BOARD_SIZE = 760

    def __init__(self, root: Tk) -> None:
        # root window
        self.root = root
        self.root.title("Conway's Game of Life")

        # screen drawn variables
        self.backend = Backend()
        self.type_var = StringVar(value="Square")
        self.size_var = StringVar(value="25 x 25")
        self.delay_var = DoubleVar(value=150.0)
        self.status_var = StringVar(value="Paused")
        self.generation_var = StringVar(value="Generation: 0")
        self.birth_var = IntVar(value=RECT_DEFAULT_BIRTH)
        self.survival_min_var = IntVar(value=RECT_DEFAULT_SURVIVAL_MIN)
        self.survival_max_var = IntVar(value=RECT_DEFAULT_SURVIVAL_MAX)

        # other attributes
        self.running = False
        self.grid_type = GridType.RECT
        self.next_ticks_id: str | None = None  # next tkinter task id
        self.generations = ctypes.c_ulong(0)

        # grid setup
        self.width, self.height = self.SIZES[self.size_var.get()]
        self.grid = self.backend.malloc_grid(self.width, self.height)
        self.backend.empty_grid(self.grid, self.width, self.height)

        # bimap grid coord <-> id
        # coordinate to id
        self.cell_items: dict[tuple[int, int], int] = {}
        # id to coordinate
        self.item_to_cell: dict[int, tuple[int, int]] = {}

        self._build_ui()
        self._draw_board()
        self._refresh_board()
        # free() and stuff on exit
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_ui(self) -> None:  # draw ui
        outer = ttk.Frame(self.root, padding=12)
        outer.pack(fill=BOTH, expand=True)

        self.canvas = Canvas(
            outer,
            width=self.DRAWN_BOARD_SIZE,
            height=self.DRAWN_BOARD_SIZE,
            background=BACKGROUND,
            highlightthickness=0,
        )
        self.canvas.pack(side=LEFT, fill=BOTH, expand=True)
        # handle cell clikc
        self.canvas.bind("<Button-1>", self._on_canvas_click)
        # handle resizing
        self.canvas.bind("<Configure>", self._on_canvas_resized)

        # main control panel frame
        control_panel = ttk.Frame(outer, padding=(12, 0, 0, 0))
        control_panel.pack(side=RIGHT, fill=Y)

        # choosing board type
        ttk.Label(control_panel, text="Board Type").pack(anchor="w")
        type_box = ttk.Combobox(
            control_panel,
            textvariable=self.type_var,
            values=("Square", "Hexagonal"),
            state="readonly",
            width=16,
        )

        type_box.pack(anchor="w", fill="x", pady=(0, 12))
        type_box.bind("<<ComboboxSelected>>", self._on_type_changed)

        # selecting size
        ttk.Label(control_panel, text="Board Size").pack(anchor="w")

        size_box = ttk.Combobox(
            control_panel,
            textvariable=self.size_var,
            values=tuple(self.SIZES.keys()),
            state="readonly",
            width=16,
        )

        size_box.pack(anchor="w", fill="x", pady=(0, 12))
        size_box.bind("<<ComboboxSelected>>", self._on_size_changed)

        # generation count label
        ttk.Label(control_panel, textvariable=self.generation_var).pack(
            anchor="w", pady=(0, 6)
        )

        # status label
        ttk.Label(control_panel, textvariable=self.status_var).pack(
            anchor="w", pady=(0, 12)
        )

        # other buttons
        ttk.Button(
            control_panel, text="Reset Board", command=self._reset_current_board
        ).pack(fill="x", pady=2)
        ttk.Button(
            control_panel, text="Randomise", command=self._randomise_board
        ).pack(fill="x", pady=2)
        self.pause_button = ttk.Button(
            control_panel, text="Start", command=self._toggle_run
        )
        self.pause_button.pack(fill="x", pady=2)
        ttk.Button(control_panel, text="Step", command=self._step_once).pack(
            fill="x", pady=2
        )
        ttk.Button(
            control_panel, text="Import", command=self._import_board
        ).pack(fill="x", pady=2)
        ttk.Button(
            control_panel, text="Export", command=self._export_board
        ).pack(fill="x", pady=2)

        ttk.Label(control_panel, text="Delay (ms)").pack(
            anchor="w", pady=(14, 0)
        )
        ttk.Scale(
            control_panel,
            from_=25,
            to=1000,
            variable=self.delay_var,
            orient="horizontal",
        ).pack(fill="x", pady=(0, 6))
        # B/S
        ttk.Label(control_panel, text="Birth Count").pack(
            anchor="w", pady=(14, 0)
        )
        ttk.Spinbox(
            control_panel,
            from_=0,
            to=8,
            textvariable=self.birth_var,
            width=14,
        ).pack(anchor="w", fill="x", pady=(0, 6))

        ttk.Label(control_panel, text="Survival Min").pack(anchor="w")
        ttk.Spinbox(
            control_panel,
            from_=0,
            to=8,
            textvariable=self.survival_min_var,
            width=14,
        ).pack(anchor="w", fill="x", pady=(0, 6))

        ttk.Label(control_panel, text="Survival Max").pack(anchor="w")
        ttk.Spinbox(
            control_panel,
            from_=0,
            to=8,
            textvariable=self.survival_max_var,
            width=14,
        ).pack(anchor="w", fill="x", pady=(0, 6))

    def _on_type_changed(self, _event=None) -> None:
        # changes mode
        self.grid_type = (
            GridType.RECT if self.type_var.get() == "Square" else GridType.HEX
        )
        self._set_default_rules()
        self._reset_entire_board()

    def _on_size_changed(self, _event=None) -> None:
        # change in board size
        self.width, self.height = self.SIZES[self.size_var.get()]
        self._reset_entire_board()

    def _reset_entire_board(self) -> None:
        # reset board, free old, alloc new, clear, reset gens, redraw
        self._stop_running()
        self.backend.free_grid(self.grid)
        self.grid = self.backend.malloc_grid(self.width, self.height)
        self.backend.empty_grid(self.grid, self.width, self.height)
        self.generations = ctypes.c_ulong(0)
        self._draw_board()
        self._refresh_board()

    def _reset_current_board(self) -> None:
        # stop sim, keep same grid, empty grid, reset gen, and refresh board
        self._stop_running()
        self.backend.empty_grid(self.grid, self.width, self.height)
        self.generations = ctypes.c_ulong(0)
        self._refresh_board()

    def _randomise_board(self) -> None:
        # reset board and refresh
        self.backend.random_grid(self.grid, self.width, self.height)
        self.generations = ctypes.c_ulong(0)
        self._refresh_board()

    def _toggle_run(self) -> None:
        # run if stopped, stop if run
        if self.running:
            self._stop_running()
        else:
            self.running = True
            self.status_var.set("Running")
            self.pause_button.config(text="Pause")
            self._schedule_next_tick()

    def _stop_running(self) -> None:
        # stop running
        self.running = False
        self.status_var.set("Paused")
        self.pause_button.config(text="Start")

        if self.next_ticks_id is not None:
            self.root.after_cancel(self.next_ticks_id)
            self.next_ticks_id = None

    def _schedule_next_tick(self) -> None:
        if not self.running:
            return  # if we're not running, no need to schedule next tick
        delay = max(1, int(self.delay_var.get()))  # min speed of 1

        self.next_ticks_id = self.root.after(delay, self._tick)

    def _tick(self) -> None:
        # main tick method
        # step one tick through, and schedule next tick
        self.next_ticks_id = None
        if not self.running:
            return

        self.backend.update_grid(
            self.grid,
            self.width,
            self.height,
            self.generations,
            self.grid_type,
            self._current_rules(),
        )

        self._refresh_board()
        self._schedule_next_tick()

    def _step_once(self) -> None:
        # step forward one tick
        if self.running:
            return  # do nothing if we're already running
        self.backend.update_grid(
            self.grid,
            self.width,
            self.height,
            self.generations,
            self.grid_type,
            self._current_rules(),
        )
        self._refresh_board()

    # default hex and rect rules
    def _set_default_rules(self) -> None:
        if self.grid_type == GridType.RECT:
            self.birth_var.set(RECT_DEFAULT_BIRTH)
            self.survival_min_var.set(RECT_DEFAULT_SURVIVAL_MIN)
            self.survival_max_var.set(RECT_DEFAULT_SURVIVAL_MAX)
        else:
            self.birth_var.set(HEX_DEFAULT_BIRTH)
            self.survival_min_var.set(HEX_DEFAULT_SURVIVAL_MIN)
            self.survival_max_var.set(HEX_DEFAULT_SURVIVAL_MAX)

    # update ruleset
    def _current_rules(self) -> RuleSet:
        survival_min = self.survival_min_var.get()
        survival_max = self.survival_max_var.get()
        if survival_min > survival_max:
            survival_min, survival_max = survival_max, survival_min

        return RuleSet(
            birth_count=self.birth_var.get(),
            survival_min=survival_min,
            survival_max=survival_max,
        )

    def _on_canvas_resized(self, _event=None) -> None:
        # to ensure scaling
        self._draw_board()
        self._refresh_board()

    def _draw_board(self) -> None:
        # delete current canvas, clear, and draw correct board type
        self.canvas.delete("all")
        self.cell_items.clear()
        self.item_to_cell.clear()

        if self.grid_type == GridType.RECT:
            self._draw_square_board()
        else:
            self._draw_hex_board()

    def _draw_square_board(self) -> None:
        canvas_w, canvas_h = self._canvas_dimensions()
        # cells are square with side length = min(possible height, 
        # possible width)
        cell_size = min(canvas_w / self.width, canvas_h / self.height)

        board_width = cell_size * self.width
        board_height = cell_size * self.height

        # for cenntering the board
        offset_x = (canvas_w - board_width) / 2
        offset_y = (canvas_h - board_height) / 2

        for x in range(self.height):
            for y in range(self.width):
                x0 = offset_x + y * cell_size  # start x pos for this cell
                y0 = offset_y + x * cell_size  # start y pos for this cell

                item = self.canvas.create_rectangle(
                    x0,
                    y0,
                    x0 + cell_size,
                    y0 + cell_size,
                    fill=BOARD_COL,  # cream ish
                    outline=OUTLINE_COL,  # another cream
                )
                # add to bimap
                self.cell_items[(x, y)] = item
                self.item_to_cell[item] = (x, y)

    def _draw_hex_board(self) -> None:
        # draws the board interpreted in hex form
        # underlying board is still rectangular

        # chooses a hexagon size that fits the screen
        # then gets the center of the hexagon
        # then gets the 6 corners using hex points, and then draws it
        # still using odd-r

        canvas_w, canvas_h = self._canvas_dimensions()
        sqrt3 = math.sqrt(3.0)  # stored so we don't recalculate it

        # max width of sqrt(3) * self.width, but staggered so + 0.5
        width_size = canvas_w / (sqrt3 * (self.width + 0.5))

        # top and bottom need extra half so +0.5 at the end
        height_size = canvas_h / ((1.5 * self.height) + 0.5)

        size = min(width_size, height_size)
        hex_width = sqrt3 * size
        board_width = hex_width * (self.width + 0.5)

        vstep = 1.5 * size  # vertical distance to next row's hexagon centre
        # first row 2* size + vstep * (number of rows - 1)
        board_height = 2 * size + vstep * (self.height - 1)
        # find centre of the board
        offset_x = (canvas_w - board_width) / 2 + hex_width / 2
        offset_y = (canvas_h - board_height) / 2 + size

        for x in range(self.height):
            row_offset = (hex_width / 2) if (x & 1) else 0.0  # odd-r check
            centre_y = offset_y + x * vstep  # centre of this cell
            for y in range(self.width):
                centre_x = offset_x + row_offset + y * hex_width
                points = self._hex_points(
                    centre_x, centre_y, size
                )  # hexagon points
                hexagon = self.canvas.create_polygon(
                    points, fill=DEAD_COL, outline=OUTLINE_COL
                )
                # update bimap
                self.cell_items[(x, y)] = hexagon
                self.item_to_cell[hexagon] = (x, y)

    @staticmethod  # class method
    def _hex_points(cx: float, cy: float, size: float) -> list[float]:
        # computes the vertices for one hexagon to draw it
        points = []
        for i in range(6):
            angle = math.radians(60 * i - 30)  # convert to radians
            # - 30 to tilt it so its pointy top
            points.extend(
                (cx + size * math.cos(angle), cy + size * math.sin(angle))
            )
        return points

    def _canvas_dimensions(self) -> tuple[int, int]:
        # sets canvas dimensions
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            return self.DRAWN_BOARD_SIZE, self.DRAWN_BOARD_SIZE
        return width, height

    def _refresh_board(self) -> None:
        for x in range(self.height):
            for y in range(self.width):
                status = self.grid[x * self.width + y].status
                fill = ALIVE_COL if status else DEAD_COL
                self.canvas.itemconfigure(self.cell_items[(x, y)], fill=fill)
        self.generation_var.set(f"Generation: {self.generations.value}")

    def _on_canvas_click(self, _event) -> None:
        # try modify to get dragging the mouse to work?
        current = self.canvas.find_withtag("current")
        if not current:  # no tkinter thingy there
            return

        cell = self.item_to_cell[current[0]]
        if cell is None:
            return
        x, y = cell
        index = (x * self.width) + y
        self.grid[index].status = (
            DEAD if (self.grid[index].status == ALIVE) else ALIVE
        )
        self.canvas.itemconfigure(
            self.cell_items[(x, y)],
            fill=ALIVE_COL if self.grid[index].status else DEAD_COL,
        )

    def _import_board(self) -> None:
        # allow user to import a board
        path = filedialog.askopenfilename(
            title="Import board",
            filetypes=(
                ("Game of Life board", "*.gol *.txt"),
                ("All files", "*.*"),
            ),
        )
        if not path:  # None/ empty path
            return

        grid_info = self.backend.load_grid(path)
        if grid_info is None:
            messagebox.showerror(
                "Import failed", "Could not load the selected board file."
            )
            return

        self._stop_running()
        self.backend.free_grid(self.grid)
        self.grid, self.width, self.height, self.grid_type = grid_info

        self.generations = ctypes.c_ulong(0)
        self.type_var.set(
            "Square" if self.grid_type == GridType.RECT else "Hexagonal"
        )
        self._set_default_rules()
        size_label = f"{self.width} x {self.height}"
        if size_label in self.SIZES:
            self.size_var.set(size_label)
        else:  # should be unreachable, just for debugging
            raise ValueError(f"Invalid label size: {size_label!r}")
        self._draw_board()
        self._refresh_board()

    def _export_board(self) -> None:
        path = filedialog.asksaveasfilename(
            title="Export board",
            defaultextension=".gol",  # custom file type
            filetypes=(
                ("Game of Life board", "*.gol"),
                ("Text file", "*.txt"),
                ("All files", "*.*"),
            ),
        )
        if not path:  # path is None / empty
            return

        success = self.backend.save_grid(
            path, self.grid, self.width, self.height, self.grid_type
        )
        if not success:
            messagebox.showerror(
                "Export failed", "Could not save the board to that path."
            )

    def _on_close(self) -> None:
        self._stop_running()
        self.backend.free_grid(self.grid)
        self.root.destroy()


def main() -> None:
    root = Tk()
    style = ttk.Style(root)
    theme: str = "clam"  # bit dull, maybe switch out for smth more clean
    if theme in style.theme_names():
        style.theme_use(theme)
    app = GameOfLife(root)

    root.mainloop()


if __name__ == "__main__":
    # we're not importing this in c, so this will be how the program runs
    main()
