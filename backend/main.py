from fastapi import FastAPI, HTTPException
from dotenv import load_dotenv

load_dotenv()          # ← до імпорту db, щоб змінні вже були в середовищі
import db

app = FastAPI(title="IoT Backend")

@app.get("/health")
def health():
    return {"status": "ok"}

@app.get("/sensors/latest")
def sensors_latest():
    item = db.get_latest()
    if item is None:
        raise HTTPException(status_code=404, detail="No data yet")
    return item

@app.get("/sensors/history")
def sensors_history(minutes: int = 30):
    return db.get_history(minutes)