#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

sns.set(style="whitegrid", font_scale=1.1)

# ===============================
# CONFIGURACIÓN
# ===============================
FILES = {
    "linked_list": "resultados_linked_list.csv",
    "matvec": "resultados_matvec.csv",
    "barrier": "resultados_barrier.csv",
}
OUT_DIR = "graficos"
os.makedirs(OUT_DIR, exist_ok=True)

# ===============================
# FUNCIÓN AUXILIAR DE GRÁFICA
# ===============================
def plot_comparison(df, x, y, hue, title, ylabel, outfile):
    plt.figure(figsize=(8, 5))
    sns.lineplot(data=df, x=x, y=y, hue=hue, marker="o")
    plt.title(title)
    plt.ylabel(ylabel)
    plt.xlabel(x)
    plt.legend(title="Implementación")
    plt.tight_layout()
    plt.savefig(os.path.join(OUT_DIR, outfile))
    plt.close()
    print(f"✔ Gráfico guardado: {outfile}")

# ===============================
# 1️⃣ LINKED LIST
# ===============================
if os.path.exists(FILES["linked_list"]):
    df_ll = pd.read_csv(FILES["linked_list"])
    df_ll["impl_type"] = df_ll["impl"].apply(
        lambda s: "Propia (Lock-Free)" if "lockfree" in s else "Libro (Mutex Global)"
    )
    plot_comparison(
        df_ll,
        x="threads",
        y="time_ms",
        hue="impl_type",
        title="Lista Enlazada Multithread",
        ylabel="Tiempo [ms]",
        outfile="lista_enlazada.png",
    )

    resumen_ll = (
        df_ll.groupby(["impl_type", "threads"])["time_ms"]
        .agg(["mean", "min", "max"])
        .reset_index()
    )
else:
    print("⚠ No se encontró resultados_linked_list.csv")

# ===============================
# 2️⃣ MATVEC
# ===============================
if os.path.exists(FILES["matvec"]):
    df_mv = pd.read_csv(FILES["matvec"])
    df_mv["impl_type"] = df_mv["impl"].apply(
        lambda s: "Propia (Dinámica)" if "dynamic" in s else "Libro (Estática)"
    )
    df_mv["threads"] = df_mv["threads"].astype(int)

    # Gráfico general comparando tiempos por hilos
    plot_comparison(
        df_mv,
        x="threads",
        y="time_ms",
        hue="impl_type",
        title="Multiplicación Matriz-Vector",
        ylabel="Tiempo [ms]",
        outfile="matvec.png",
    )

    resumen_mv = (
        df_mv.groupby(["impl_type", "threads"])["time_ms"]
        .agg(["mean", "min", "max"])
        .reset_index()
    )
else:
    print("⚠ No se encontró resultados_matvec.csv")

# ===============================
# 3️⃣ BARRIER / THREAD SAFETY
# ===============================
if os.path.exists(FILES["barrier"]):
    df_b = pd.read_csv(FILES["barrier"])
    df_b["impl_type"] = df_b["impl"].apply(
        lambda s: "Propia (Sense-Reversing + Ticket)" if "sense" in s else "Libro (CondVar)"
    )
    plot_comparison(
        df_b,
        x="threads",
        y="time_ms",
        hue="impl_type",
        title="Sincronización y Seguridad en Hilos",
        ylabel="Tiempo [ms]",
        outfile="barrier.png",
    )

    resumen_b = (
        df_b.groupby(["impl_type", "threads"])["time_ms"]
        .agg(["mean", "min", "max"])
        .reset_index()
    )
else:
    print("⚠ No se encontró resultados_barrier.csv")

# ===============================
# 4️⃣ ANÁLISIS COMPARATIVO
# ===============================
with open("reporte_analisis.txt", "w") as f:
    f.write("=== ANÁLISIS DE RESULTADOS ===\n\n")

    if "resumen_ll" in locals():
        f.write("📘 Lista Enlazada Multithread:\n")
        f.write(resumen_ll.to_string(index=False))
        f.write("\n\n")
        speedup_ll = (
            df_ll.pivot(index="threads", columns="impl_type", values="time_ms")
            .apply(lambda x: x.min() / x)
        )
        f.write("Speedup relativo (menor tiempo como referencia):\n")
        f.write(speedup_ll.to_string())
        f.write("\n\n")

    if "resumen_mv" in locals():
        f.write("📗 Matriz × Vector:\n")
        f.write(resumen_mv.to_string(index=False))
        f.write("\n\n")
        speedup_mv = (
            df_mv.pivot_table(index="threads", columns="impl_type", values="time_ms", aggfunc="mean")
            .apply(lambda x: x.min() / x)
        )
        f.write("Speedup relativo (menor tiempo como referencia):\n")
        f.write(speedup_mv.to_string())
        f.write("\n\n")

    if "resumen_b" in locals():
        f.write("📙 Thread Safety / Barreras:\n")
        f.write(resumen_b.to_string(index=False))
        f.write("\n\n")
        speedup_b = (
            df_b.pivot(index="threads", columns="impl_type", values="time_ms")
            .apply(lambda x: x.min() / x)
        )
        f.write("Speedup relativo (menor tiempo como referencia):\n")
        f.write(speedup_b.to_string())
        f.write("\n\n")

    f.write("=== FIN DEL ANÁLISIS ===\n")

print("📄 Se generó 'reporte_analisis.txt' con tablas de resumen y speedups.")
print("📊 Gráficos en carpeta:", OUT_DIR)
