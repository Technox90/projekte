"""Maintain a single AzuraCast now-playing embed inside a Discord voice channel chat."""
import asyncio
import json
import logging
import os
import re
from pathlib import Path

import aiohttp
import discord

log = logging.getLogger("bbq-radio.nowplaying")
API_URL = os.getenv("AZURACAST_NOWPLAYING_URL", "https://stream.bopzocker.de/api/nowplaying/chaos")
PUBLIC_URL = "https://stream.bopzocker.de/public/chaos"
MESSAGE_FILE = Path(__file__).with_name("nowplaying-message.json")
POLL_SECONDS = 20


def clean_title(value):
    return re.sub(
        r"\s*[|｜]\s*(?:(?:HQ|HD|4K|8K|UHD|FHD|1080p|720p|60fps)\s*)*(?:official\s+)?(?:music\s+)?video(?:clip)?\b.*$",
        "", str(value or ""), flags=re.IGNORECASE
    ).strip()


def build_embed(data):
    now = data.get("now_playing") or {}
    song = now.get("song") or {}
    title = clean_title(song.get("title") or "Unbekannter Titel")[:256]
    artist = str(song.get("artist") or "Unbekannter Interpret")[:256]
    listeners = data.get("listeners") or {}
    amount = listeners.get("total", 0)
    online = bool(data.get("is_online"))
    dj_live = bool((data.get("live") or {}).get("is_live"))
    if not online:
        status = "⚫ Offline"
    elif dj_live:
        status = "🔴 Live"
    else:
        status = "🟢 Online"
    embed = discord.Embed(
        title="🎵 BBQ CHAOS | LIVE RADIO",
        description=f"**{title}**\n🎤 {artist}",
        url=PUBLIC_URL,
        colour=discord.Colour.red() if dj_live and online else (discord.Colour.green() if online else discord.Colour.dark_grey()),
    )
    embed.add_field(name="📡 Status", value=status, inline=False)
    embed.add_field(name="👥 Hörer", value=str(amount), inline=False)
    embed.add_field(name="🌐 Direkt hören", value=f"[Radio öffnen]({PUBLIC_URL})", inline=False)
    art = song.get("art")
    if isinstance(art, str) and art.startswith("https://"):
        embed.set_thumbnail(url=art)
    embed.set_footer(text="BBQ CHAOS • Gemeinsam zocken, gemeinsam Musik hören")
    return embed


async def run_nowplaying(bot, channel_resolver):
    """Poll metadata, edit the same Discord message. Independent of voice handshake."""
    await bot.wait_until_ready()
    last_signature = None
    message_id = None
    if MESSAGE_FILE.exists():
        try:
            saved = json.loads(MESSAGE_FILE.read_text(encoding="utf-8"))
            if saved.get("channel_id") == int(os.environ["DISCORD_CHANNEL_ID"]):
                message_id = int(saved["message_id"])
        except (ValueError, KeyError, OSError, TypeError):
            log.warning("Gespeicherte Nachrichten-ID konnte nicht gelesen werden")
    timeout = aiohttp.ClientTimeout(total=12)
    async with aiohttp.ClientSession(timeout=timeout) as session:
        while not bot.is_closed():
            try:
                async with session.get(API_URL) as response:
                    response.raise_for_status()
                    data = await response.json(content_type=None)
                if not isinstance(data, dict):
                    raise ValueError("AzuraCast antwortet nicht mit einem JSON-Objekt")
                now = data.get("now_playing") or {}
                song = now.get("song") or {}
                listeners = data.get("listeners") or {}
                signature = (
                    str(song.get("id") or song.get("title") or ""),
                    str(song.get("artist") or ""),
                    str(listeners.get("total", 0)),
                    bool(data.get("is_online")),
                    bool((data.get("live") or {}).get("is_live")),
                )
                if signature != last_signature:
                    channel = await channel_resolver()
                    embed = build_embed(data)
                    message = None
                    if message_id:
                        try:
                            message = await channel.fetch_message(message_id)
                            if message.author.id != bot.user.id:
                                raise RuntimeError("Gespeicherte Nachricht gehoert nicht dem Bot")
                            await message.edit(embed=embed)
                        except discord.NotFound:
                            message_id = None
                        # Forbidden and other errors are NOT a reason to create duplicates.
                    if message_id is None:
                        message = await channel.send(embed=embed, silent=True)
                        message_id = message.id
                        tmp = MESSAGE_FILE.with_suffix(".tmp")
                        tmp.write_text(json.dumps({"channel_id": channel.id, "message_id": message_id}), encoding="utf-8")
                        tmp.replace(MESSAGE_FILE)
                    last_signature = signature
                    log.info("NowPlaying aktualisiert: %s - %s", song.get("artist"), song.get("title"))
            except asyncio.CancelledError:
                raise
            except Exception:
                log.exception("NowPlaying-Aktualisierung fehlgeschlagen; Wiederholung in %ss", POLL_SECONDS)
            await asyncio.sleep(POLL_SECONDS)
