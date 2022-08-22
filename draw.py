
import plotly.graph_objects as go
import os

# Spyder 编辑器加上下面两行代码
# import plotly.io as pio
# print(pio.renderers)
# pio.renderers.default = 'pdf'
# print(pio.renderers.default)


import tools

def draw_customers(path):

    schools = ["Brown", "NYU", "Notre Dame", "Cornell", "Tufts", "Yale",
            "Dartmouth", "Chicago", "Columbia", "Duke", "Georgetown",
            "Princeton", "U.Penn", "Stanford", "MIT", "Harvard"]

    fig = go.Figure()
    inst = tools.read_vrplib(path)
    # print(inst.keys())
    # print(inst["coords"])

    xx = [c[0] for c in inst["coords"]]
    yy = [c[1] for c in inst["coords"]]

    fig.add_trace(go.Scatter(
        x=xx,
        y=yy,
        marker=dict(color="crimson", size=12),
        mode="markers",
        name="Women",
    ))


    fig.update_layout(title="Gender Earnings Disparity",
                    xaxis_title="Annual Salary (in thousands)",
                    yaxis_title="School")

    inst_name = path.split('/')[1].split('.')[0]
    fig.write_html(f'./draws/{inst_name}.html')

if __name__ == "__main__":

    path = r'instances'
    for filename in os.listdir(path):
        pt = os.path.join(path, filename)
        print(f"draw instance:{pt}")
        draw_customers(path = pt)

    # path = 'instances/ORTEC-VRPTW-ASYM-0bdff870-d1-n458-k35.txt'
    # draw_customers(path =path)