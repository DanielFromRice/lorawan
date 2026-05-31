import pandas as pd

df = pd.read_csv("../../combined_results.csv")

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

df_agg.to_csv("../../aggregate_results.csv")