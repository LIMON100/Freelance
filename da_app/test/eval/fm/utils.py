import cv2
import numpy as np


def draw_table_grid(img, x_offset, y_offset, rows, column_widths, row_height=30, padding=5,
                    grid_color=(0, 255, 0), text_color=(255, 255, 255), thickness=1, font_scale=0.3,
                    font=cv2.FONT_HERSHEY_SIMPLEX, alignment='left'):

    # Draw horizontal lines
    for i in range(len(rows) + 1):
        cv2.line(img, (x_offset, y_offset + i * row_height),
                 (x_offset + sum(column_widths), y_offset + i * row_height), grid_color, thickness)

    # Draw vertical lines
    for i in range(len(column_widths) + 1):
        x = x_offset + sum(column_widths[:i])
        cv2.line(img, (x, y_offset), (x, y_offset + len(rows) * row_height), grid_color, thickness)

    # Insert text into each cell
    for i, row in enumerate(rows):
        for j, cell_text in enumerate(row):
            x = x_offset + sum(column_widths[:j]) + padding
            y = y_offset + i * row_height + row_height - padding
            (text_width, text_height), _ = cv2.getTextSize(cell_text, font, font_scale, thickness)

            if alignment == 'left':
                text_x = x
            elif alignment == 'center':
                text_x = x + (column_widths[j] - text_width) // 2
            elif alignment == 'right':
                text_x = x + column_widths[j] - text_width - padding
            else:
                raise ValueError("Alignment must be 'left', 'center', or 'right'")

            text_y = y - (row_height - text_height) // 2
            cv2.putText(img, cell_text, (text_x, text_y + 10), font, font_scale, text_color, thickness)

def scale_up_frame(frame, scale_factor):
    height, width = frame.shape[:2]
    new_width = int(width * scale_factor)
    new_height = int(height * scale_factor)
    return cv2.resize(frame, (new_width, new_height), interpolation=cv2.INTER_AREA)

def sharpen_image(image, intensity=1.0):
    sharpening_kernel = np.array([[0, -1, 0],
                                    [-1, 5 + intensity - 1, -1],
                                    [0, -1, 0]])
    return cv2.filter2D(image, -1, sharpening_kernel)