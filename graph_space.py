#!/usr/bin/env python3
"""Generate Plotly charts for the space-complexity demo -- one command."""

import csv
import subprocess
import sys
import webbrowser
from pathlib import Path

try:
    import plotly.graph_objects as go
except ImportError:
    print("plotly not found -- installing...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "plotly"])
    import plotly.graph_objects as go

REPO_DIR = Path(__file__).resolve().parent
BUILD_DIR = REPO_DIR / "out" / "build" / "x64-debug"
CSV_FILE = REPO_DIR / "results_space.csv"
OUTPUT_HTML = REPO_DIR / "charts_space.html"
EXE_NAME = "big-o-demo-hash-tables-space"


# ── Build & Run ──────────────────────────────────────────────────────────────

def find_executable():
    """Locate the compiled executable across common build layouts."""
    out_build = REPO_DIR / "out" / "build"
    if out_build.exists():
        for exe in out_build.rglob(f"{EXE_NAME}.exe"):
            if "CompilerId" not in str(exe):
                return exe

    candidates = [
        REPO_DIR / "build" / f"{EXE_NAME}.exe",
        REPO_DIR / "build" / "Debug" / f"{EXE_NAME}.exe",
        REPO_DIR / "build" / "Release" / f"{EXE_NAME}.exe",
        REPO_DIR / "build" / EXE_NAME,
    ]
    for path in candidates:
        if path.exists():
            return path
    return None


def build_and_run():
    exe = find_executable()
    if exe is None:
        print("No executable found -- building with CMake...")
        build_dir = REPO_DIR / "build"
        subprocess.run(["cmake", "-B", str(build_dir), str(REPO_DIR)], check=True)
        subprocess.run(["cmake", "--build", str(build_dir)], check=True)
        exe = find_executable()
        if exe is None:
            sys.exit("ERROR: build succeeded but executable not found")

    print(f"Running {exe.name}...\n")
    subprocess.run([str(exe)], cwd=str(REPO_DIR), check=True)


# ── Parse CSV ─────────────────────────────────────────────────────────────────

def read_results():
    with open(CSV_FILE) as f:
        return list(csv.DictReader(f))


# ── Chart Generation ──────────────────────────────────────────────────────────

COLORS = {
    "Chaining": "#2563eb",  # blue
    "Probing":  "#dc2626",  # red
}


def build_memory_growth_chart(rows):
    """Memory vs n for both structures."""
    fig = go.Figure()

    for struct in ("Chaining", "Probing"):
        data = [r for r in rows
                if r["measurement"] == "memory" and r["structure"] == struct]
        if not data:
            continue
        fig.add_trace(go.Scatter(
            x=[int(r["n"]) for r in data],
            y=[float(r["memory_kb"]) for r in data],
            mode="lines+markers",
            name=f"{struct} -- O(n)",
            line=dict(color=COLORS.get(struct, "#3b82f6"), width=3),
            marker=dict(size=10),
        ))

    fig.update_layout(
        title=dict(text="Memory Growth -- O(n)", font=dict(size=22)),
        xaxis_title="n (elements inserted)",
        yaxis_title="Estimated Memory (KB)",
        yaxis=dict(rangemode="tozero"),
        template="plotly_white",
        font=dict(size=14),
        legend=dict(font=dict(size=14)),
        margin=dict(t=60, b=60),
        height=450,
    )
    return fig


def build_load_factor_chart(rows):
    """Stacked bar: entry memory vs empty-bucket overhead.

    For chaining, memory = bucket_array + node_memory.
    - bucket_array = capacity * bucket_size (scales with capacity, wasted when empty)
    - node_memory = n * node_size (constant for same n -- this is the "used" part)

    At different load factors, n is the same but capacity = n/LF changes.
    So the entry cost (green) is the same in every bar, and the bucket
    overhead (red) grows as LF drops (bigger table = more buckets).
    """
    data = [r for r in rows if r["measurement"] == "load_factor_memory"]
    if not data:
        return None

    lfs = []
    for r in data:
        try:
            lfs.append(float(r["complexity"].replace("LF=", "")))
        except ValueError:
            lfs.append(0)

    totals = [float(r["memory_kb"]) for r in data]
    n = int(data[0]["n"])

    # Compute entry-only cost: at LF=1.0, capacity = n, so bucket overhead
    # is minimal. We can estimate entry cost by extrapolating:
    # total = entry_cost + (capacity * bucket_overhead_per_slot)
    # At two different LFs with same n:
    #   total1 = entry_cost + (n/lf1) * B
    #   total2 = entry_cost + (n/lf2) * B
    # Solve: B = (total1 - total2) / (n/lf1 - n/lf2)
    #         entry_cost = total1 - (n/lf1) * B
    if len(lfs) >= 2 and lfs[0] != lfs[1]:
        cap1 = n / lfs[0]
        cap2 = n / lfs[1]
        B = (totals[0] - totals[1]) / (cap1 - cap2)  # KB per slot
        entry_cost = totals[0] - cap1 * B
        if entry_cost < 0:
            entry_cost = 0
    else:
        # Fallback: use highest-LF bar as approximation
        entry_cost = min(totals)

    # Now split each bar: green = entry_cost (constant), red = the rest
    used = [entry_cost] * len(totals)
    wasted = [max(0, t - entry_cost) for t in totals]

    labels = [f"{lf:.1f}" for lf in lfs]

    fig = go.Figure()

    fig.add_trace(go.Bar(
        name=f"Entries ({n:,} items)",
        x=labels,
        y=used,
        marker_color="#2ecc71",
        text=[f"{u:.0f} KB" for u in used],
        textposition="inside",
        insidetextanchor="middle",
    ))

    fig.add_trace(go.Bar(
        name="Empty bucket overhead",
        x=labels,
        y=wasted,
        marker_color="#e74c3c",
        text=[f"+{w:.0f} KB" if w > 20 else "" for w in wasted],
        textposition="inside",
        insidetextanchor="middle",
    ))

    fig.update_layout(
        barmode="stack",
        title=dict(
            text=f"Load Factor vs Memory (Chaining, n={n:,})<br>"
                 "<sup>Green = entry nodes (same n, same cost) | "
                 "Red = empty bucket overhead (bigger table = more waste)</sup>",
            font=dict(size=20)),
        xaxis_title="Load Factor",
        yaxis_title="Estimated Memory (KB)",
        yaxis=dict(rangemode="tozero"),
        template="plotly_white",
        font=dict(size=14),
        legend=dict(font=dict(size=13), orientation="h", y=-0.15, x=0.5, xanchor="center"),
        margin=dict(t=80, b=80),
        height=500,
    )
    return fig


def generate_html(rows):
    growth_chart = build_memory_growth_chart(rows)
    lf_chart = build_load_factor_chart(rows)

    # Build chart HTML fragments, sharing one copy of plotly.js
    plotly_included = False
    def chart_html(fig):
        nonlocal plotly_included
        if fig is None:
            return ""
        h = fig.to_html(full_html=False, include_plotlyjs=(not plotly_included))
        plotly_included = True
        return f'<div class="chart">{h}</div>'

    growth_div = chart_html(growth_chart)
    lf_div = chart_html(lf_chart)

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Big O Demo: Hash Tables -- Space Complexity</title>
<style>
  body {{ background: #ffffff; color: #1a1a1a; font-family: system-ui, sans-serif;
         max-width: 960px; margin: 0 auto; padding: 2rem; line-height: 1.6; }}
  h1 {{ text-align: center; margin-bottom: 0.25rem; }}
  h2 {{ margin-top: 2.5rem; border-bottom: 2px solid #e5e7eb; padding-bottom: 0.3rem; }}
  p.sub {{ text-align: center; color: #666; margin-top: 0; }}
  .chart {{ margin-bottom: 2rem; }}
  .explain {{ background: #f8f9fa; border-left: 4px solid #2563eb; padding: 1rem 1.25rem;
              margin: 1rem 0 2rem 0; border-radius: 0 6px 6px 0; }}
  .explain p {{ margin: 0.4rem 0; }}
  .key-term {{ font-weight: 600; color: #2563eb; }}
  ul {{ margin: 0.5rem 0; padding-left: 1.5rem; }}
  li {{ margin: 0.3rem 0; }}
  .tradeoff {{ display: flex; gap: 1.5rem; margin: 1rem 0; }}
  .tradeoff > div {{ flex: 1; background: #f8f9fa; padding: 1rem; border-radius: 6px;
                     border: 1px solid #e5e7eb; }}
  .tradeoff h3 {{ margin: 0 0 0.5rem 0; font-size: 1rem; }}
</style>
</head>
<body>
<h1>Big O Demo: Hash Tables -- Space Complexity</h1>
<p class="sub">How much memory do hash tables actually use, and what controls it?</p>

<div class="explain">
  <p><strong>What is space complexity?</strong>
  Time complexity measures how long an operation takes.
  <span class="key-term">Space complexity</span> measures how much <em>memory</em>
  a data structure uses as the amount of data (n) grows.</p>
  <p>For hash tables, the answer is <strong>O(n)</strong> -- memory grows linearly
  with the number of elements stored. Double the data, roughly double the memory.</p>
</div>

<h2>Chart 1: Memory Grows Linearly with n</h2>
<div class="explain">
  <p><strong>What to look for:</strong> Both lines go up in a straight line.
  That's O(n) -- each new element adds a roughly constant amount of memory.</p>
  <p><strong>Why two lines?</strong> Chaining and linear probing store data differently:</p>
  <ul>
    <li><strong>Chaining</strong> -- each bucket is a linked list. Every element
    gets its own list node with two extra pointers (prev &amp; next). More overhead per element.</li>
    <li><strong>Linear Probing</strong> -- uses flat arrays. No per-element pointer overhead,
    but every slot in the array is allocated whether it's used or not.</li>
  </ul>
  <p>The gap between the lines shows the <em>overhead cost</em> of each strategy.</p>
</div>
{growth_div}

<h2>Chart 2: The Space-Time Tradeoff (Load Factor)</h2>
<div class="explain">
  <p><strong>What is load factor?</strong>
  <span class="key-term">Load factor = n / capacity</span> -- how full the table is.
  If you store 10,000 items in a table with 20,000 slots, the load factor is 0.5 (50% full).</p>
  <p><strong>What to look for:</strong> The bars get <em>shorter</em> as load factor goes up.
  The load factor itself doesn't allocate memory -- it's just a ratio. What happens is:
  when the load factor crosses the threshold, <strong>resize</strong> fires and allocates a <strong>new, larger table</strong>.
  After resize, the load factor drops because the same entries now live in a bigger table.</p>
  <p><strong>The result:</strong> a table with a low load factor has lots of <strong>empty slots</strong> --
  allocated but unused. At LF 0.3, 70% of slots sit empty. Those empty slots are what keep
  lookups fast (short probe sequences, short chains), but they cost memory.
  This is the fundamental tradeoff:</p>
</div>
<div class="tradeoff">
  <div>
    <h3>Low load factor (0.3)</h3>
    <ul>
      <li>Table resizes to LARGER capacity</li>
      <li>70% of slots sit empty (wasted memory)</li>
      <li>But lookups are FAST (few collisions)</li>
    </ul>
  </div>
  <div>
    <h3>High load factor (0.9)</h3>
    <ul>
      <li>Table stays at SMALLER capacity</li>
      <li>Only 10% of slots are empty (efficient)</li>
      <li>But lookups are SLOWER (many collisions)</li>
    </ul>
  </div>
</div>
{lf_div}

<div class="explain" style="border-left-color: #059669; margin-top: 2.5rem;">
  <p><strong>The big picture:</strong> Hash tables use O(n) memory. The load factor threshold
  controls when resize happens, which determines how big the table gets relative to
  the data. A lower threshold means resize fires sooner, creating a bigger table with
  more empty slots -- faster lookups, but more wasted memory. A higher threshold lets
  the table fill up more before resizing -- less waste, but slower operations.
  Resize itself costs O(n) time and briefly uses ~2x memory while both old and new arrays exist.</p>
  <p>This space-time tradeoff is one of the most important ideas in data structures.</p>
</div>

</body>
</html>"""

    OUTPUT_HTML.write_text(html, encoding="utf-8")
    print(f"\nCharts written to {OUTPUT_HTML}")
    webbrowser.open(OUTPUT_HTML.as_uri())


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    if "--graph-only" not in sys.argv:
        build_and_run()
    else:
        if not CSV_FILE.exists():
            sys.exit(f"ERROR: {CSV_FILE} not found -- run without --graph-only first")
        print("Skipping build/run -- using existing results_space.csv")
    rows = read_results()
    generate_html(rows)
