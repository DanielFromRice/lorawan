import pandas as pd

df = pd.read_csv("../../combined_results.csv")
df7 = pd.read_csv("../../combined_results_sf7.csv")

df.columns = df.columns.str.strip()
df.replace(" -nan", float("nan")).fillna(0)
df["lossA"] = pd.to_numeric(df['lossA'], errors='coerce').fillna(0.0)
df["lossB"] = pd.to_numeric(df['lossB'], errors='coerce').fillna(0.0)
print(df.columns)
print(df.dtypes)
group_cols = ["periodA", "nodesA", "periodB", "nodesB", "pktsizeB"]
avg_cols = ["sentA", "recvA", "lossA", "sentB", "recvB", "lossB","seed"]

df_agg = df.groupby(group_cols)[avg_cols].mean().reset_index()
print(df_agg.columns)
df_agg = df_agg.drop(columns='seed')


df7.columns = df7.columns.str.strip()
df7.replace(" -nan", float("nan")).fillna(0)
df7["lossA"] = pd.to_numeric(df7['lossA'], errors='coerce').fillna(0.0)
df7["lossB"] = pd.to_numeric(df7['lossB'], errors='coerce').fillna(0.0)
print(df7.columns)
print(df7.dtypes)
group_cols = ["periodA", "nodesA", "periodB", "nodesB", "pktsizeB"]
avg_cols = ["sentA", "recvA", "lossA", "sentB", "recvB", "lossB","seed"]

df7_agg = df7.groupby(group_cols)[avg_cols].mean().reset_index()
print(df_agg.columns)
df7_agg = df7_agg.drop(columns='seed')

df7_agg["dataMode"] = -1
df_agg["dataMode"] = 1

df_agg = pd.concat([df_agg, df7_agg], ignore_index=True).reset_index()

print(df_agg)
df_agg.to_csv("../../aggregate_results.csv")