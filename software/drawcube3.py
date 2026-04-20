import cv2
import numpy as np

def detect_circle(image): # can replace this with our hardware methods
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (7, 7), 1.5)
    edges = cv2.Canny(blurred, 30, 100)
    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    edges = cv2.dilate(edges, kernel, iterations=1)
    contours, _ = cv2.findContours(edges, cv2.RETR_LIST, cv2.CHAIN_APPROX_NONE)
    best_ellipse = None
    best_score = 0

    for cnt in contours: # remove small circles
        if len(cnt) < 5: continue
        area = cv2.contourArea(cnt)
        if area < 500: continue

        ellipse = cv2.fitEllipse(cnt)
        (cx, cy), (ma, mi), angle = ellipse

        if mi < 1e-3: continue

        ellipse_area = np.pi * (ma/2) * (mi/2)
        if ellipse_area < 1: continue

        fill_ratio = min(area, ellipse_area) / max(area, ellipse_area)
        score = fill_ratio * ellipse_area

        if score > best_score:
            best_score = score
            best_ellipse = ellipse

    return best_ellipse


def build_camera_matrix(image_shape):
    h, w = image_shape[:2]
    x = np.array([
        [w, 0, w/2],
        [0, w, h/2],
        [0, 0, 1]
    ], dtype=np.float64)
    return x


def get_pts_2d(ellipse, n=32):
    (cx, cy), (ma, mi), deg = ellipse
    theta = np.deg2rad(deg)

    pts = []
    for i in range(n):
        t = 2.0 * np.pi * i/n
        x_local = ma/2.0 * np.cos(t)
        y_local = mi/2.0 * np.sin(t)
        x_img = x_local * np.cos(theta) - y_local * np.sin(theta) + cx
        y_img = x_local * np.sin(theta) + y_local * np.cos(theta) + cy
        pts.append([x_img, y_img])
    return np.array(pts, dtype=np.float64)


def get_pts_3d(radius, n=32):
    pts = []
    for i in range(n):
        t = 2.0 * np.pi * i/n
        x = radius * np.cos(t)
        y = radius * np.sin(t)
        pts.append([x, y, 0])
    return np.array(pts, dtype=np.float64)


def estimate_pose(ellipse, C, radius, n=32):
    obj_pts = get_pts_3d(radius, n)
    img_pts = get_pts_2d(ellipse, n)
    dist_coeffs = np.zeros((4, 1), dtype=np.float64)

    success, rvec, tvec = cv2.solvePnP(obj_pts, img_pts, C, dist_coeffs, flags=cv2.SOLVEPNP_ITERATIVE)
    if not success: return None, None

    rvec, tvec = cv2.solvePnPRefineLM(obj_pts, img_pts, C, dist_coeffs, rvec, tvec)

    above_pt  = np.array([[[0.0, 0.0, float(radius)]]], dtype=np.float64)
    center_pt = np.array([[[0.0, 0.0, 0.0]]], dtype=np.float64)

    proj_above,  _ = cv2.projectPoints(above_pt,  rvec, tvec, C, dist_coeffs)
    proj_center, _ = cv2.projectPoints(center_pt, rvec, tvec, C, dist_coeffs)

    if proj_above[0][0][1] > proj_center[0][0][1]: # if above_y > center_y - flip z axis
        R, _ = cv2.Rodrigues(rvec)
        R_flip = np.array([
            [1,  0,  0],
            [0, -1,  0],
            [0,  0, -1]
        ], dtype=np.float64)
        R = R @ R_flip
        rvec, _ = cv2.Rodrigues(R)

    return rvec, tvec


def build_cube_matrix(radius, z_offset=0.0):
    s = radius * 0.65  # half side length of cube
    bot = radius * 0.15 + z_offset  # height / gap between cube and circle
    top = bot + 2.0 * s # top of cube height

    cube = np.array([
        [-s, -s, bot],
        [ s, -s, bot],
        [ s,  s, bot],
        [-s,  s, bot],

        [-s, -s, top],
        [ s, -s, top],
        [ s,  s, top],
        [-s,  s, top],
    ], dtype=np.float64)
    
    return cube


def draw_cube(image, pts_2d, alpha=0.85):
    overlay = image.copy()
    cv2.addWeighted(overlay, 1-alpha, image, alpha, 0, image)

    pts = [tuple(np.round(pt).astype(int)) for pt in pts_2d]

    def edge(i, j, color, thickness=2):
        cv2.line(image, pts[i], pts[j], color, thickness, cv2.LINE_AA)

    # bottom - orange
    for i in range(4): edge(i, (i+1) % 4, (0, 140, 255), 2)
    # top - green
    for i in range(4): edge((i+4), 4 + (i+1) % 4, (80, 255, 80), 3)
    # sides - yellow
    for i in range(4): edge(i, (i+4), (255, 200, 50), 2)


def project_cube(img_path:str, real_radius_mm:float = 50.0, z_offset:float = 0.0):
    image = cv2.imread(img_path)
    if image is None: raise FileNotFoundError(f"Path invalid at: {img_path}")

    result = image.copy()

    ellipse = detect_circle(image)
    if ellipse is None: raise ValueError("No circle detected.")

    cv2.ellipse(result, ellipse, (0, 0, 255), 2, cv2.LINE_AA)

    C = build_camera_matrix(image.shape)

    rvec, tvec = estimate_pose(ellipse, C, real_radius_mm)
    if rvec is None: raise RuntimeError("solvePnP failed.")
    
    cube_3d = build_cube_matrix(real_radius_mm, z_offset=z_offset)
    dist_coeffs = np.zeros((4, 1), dtype=np.float64)    
    projected, _ = cv2.projectPoints(cube_3d, rvec, tvec, C, dist_coeffs)

    draw_cube(result, projected.reshape(-1, 2))
    cv2.drawFrameAxes(result, C, dist_coeffs, rvec, tvec, real_radius_mm * 0.6)
    return result


if __name__ == "__main__":
    result = project_cube("C:/Users/evanil/Downloads/circle1.jpg", 50.0, 10.0)
    cv2.imshow("cube", result)
    cv2.imwrite("C:/Users/evanil/Downloads/ar_cube_output5.jpg", result)
    cv2.waitKey(0)
    cv2.destroyAllWindows()