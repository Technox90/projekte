"""Discord slash commands: radio controls and AzuraCast requests."""
import asyncio
import json
import logging
import os
import time
from pathlib import Path
from urllib.parse import quote

import aiohttp
import discord
from discord import app_commands

log = logging.getLogger("bbq-radio.commands")
BASE = os.getenv("AZURACAST_API_BASE", "https://stream.bopzocker.de").rstrip("/")
STATION = os.getenv("AZURACAST_STATION", "chaos")
COOLDOWN = 600
STATE_FILE = Path(__file__).with_name("requests-state.json")
radio = app_commands.Group(name="radio", description="BBQ-Chaos-Radio steuern")
musik = app_commands.Group(name="musik", description="Musik suchen und wuenschen")


def authorized(interaction):
    perms = interaction.user.guild_permissions
    return perms.administrator or perms.manage_guild or perms.manage_messages


def correct_server(bot, interaction):
    channel = bot.get_channel(int(os.environ["DISCORD_CHANNEL_ID"]))
    return (interaction.guild is not None and channel is not None
            and interaction.guild_id == channel.guild.id)


def setup_commands(bot):
    lock = asyncio.Lock()
    cooldowns = {}
    if STATE_FILE.is_file():
        try:
            raw = json.loads(STATE_FILE.read_text(encoding="utf-8"))
            cooldowns.update({str(k): float(v) for k, v in raw.items()})
        except (OSError, ValueError, TypeError):
            log.warning("Cooldown-Datei nicht lesbar")

    def save_cooldowns():
        temp = STATE_FILE.with_suffix(".tmp")
        temp.write_text(json.dumps(cooldowns), encoding="utf-8")
        temp.replace(STATE_FILE)

    async def gate(interaction, mod=False):
        if not correct_server(bot, interaction):
            await interaction.response.send_message("Nur auf dem konfigurierten Radioserver verfuegbar.", ephemeral=True)
            return False
        if mod and not authorized(interaction):
            await interaction.response.send_message("Nur Moderatoren oder Administratoren duerfen das Radio steuern.", ephemeral=True)
            return False
        return True

    async def request_api(method, path, *, params=None):
        headers = {}
        key = os.getenv("AZURACAST_API_KEY", "").strip()
        if key:
            headers["Authorization"] = f"Bearer {key}"
        timeout = aiohttp.ClientTimeout(total=20)
        url = BASE + "/api/station/" + quote(STATION, safe="") + path
        async with aiohttp.ClientSession(timeout=timeout) as session:
            async with session.request(method, url, headers=headers, params=params) as resp:
                content = await resp.text()
                if resp.status >= 400:
                    raise RuntimeError(f"AzuraCast HTTP {resp.status}: {content[:180]}")
                return json.loads(content) if content else {}

    def entries(response):
        if isinstance(response, list):
            return response
        if isinstance(response, dict):
            return response.get("rows") or response.get("data") or []
        return []

    def describe(item):
        song = item.get("song") or {}
        media = item.get("media") or {}
        artist = str(song.get("artist") or media.get("artist") or item.get("artist") or "?")
        title = str(song.get("title") or media.get("title") or item.get("title") or "?")
        identity = item.get("request_id") or item.get("id")
        return artist, title, str(identity) if identity is not None else None

    @radio.command(name="lautstaerke", description="Discord-Lautstaerke in Prozent (0 bis 100)")
    @app_commands.describe(prozent="Lautstaerke von 0 bis 100 Prozent")
    async def volume(interaction: discord.Interaction, prozent: app_commands.Range[int, 0, 100]):
        if not await gate(interaction, mod=True):
            return
        bot.set_volume(prozent / 100)
        await interaction.response.send_message(f"🔊 Radio-Lautstaerke: **{prozent} %** (gespeichert).", ephemeral=True)

    @radio.command(name="lauter", description="Lautstaerke um 5 Prozentpunkte erhoehen")
    async def louder(interaction: discord.Interaction):
        if not await gate(interaction, mod=True):
            return
        bot.set_volume(min(1.0, bot.volume + 0.05))
        await interaction.response.send_message(f"🔊 Lautstaerke: **{round(bot.volume * 100)} %**", ephemeral=True)

    @radio.command(name="leiser", description="Lautstaerke um 5 Prozentpunkte senken")
    async def quieter(interaction: discord.Interaction):
        if not await gate(interaction, mod=True):
            return
        bot.set_volume(max(0.0, bot.volume - 0.05))
        await interaction.response.send_message(f"🔉 Lautstaerke: **{round(bot.volume * 100)} %**", ephemeral=True)

    @radio.command(name="pause", description="Discord-Radio pausieren")
    async def pause(interaction: discord.Interaction):
        if not await gate(interaction, mod=True):
            return
        bot.paused_by_user = True
        voice = bot.radio_voice()
        if voice and voice.is_playing():
            voice.pause()
        await interaction.response.send_message("⏸️ Discord-Radio pausiert.", ephemeral=True)

    @radio.command(name="weiter", description="Discord-Radio fortsetzen")
    async def resume(interaction: discord.Interaction):
        if not await gate(interaction, mod=True):
            return
        bot.paused_by_user = False
        voice = bot.radio_voice()
        if voice and voice.is_paused():
            voice.resume()
        await interaction.response.send_message("▶️ Radio wird fortgesetzt.", ephemeral=True)

    @radio.command(name="neustart", description="Discord-Audiostream neu starten")
    async def restart(interaction: discord.Interaction):
        if not await gate(interaction, mod=True):
            return
        bot.paused_by_user = False
        voice = bot.radio_voice()
        if voice and (voice.is_playing() or voice.is_paused()):
            voice.stop()
        await interaction.response.send_message("🔄 Stream wird neu aufgebaut.", ephemeral=True)

    @radio.command(name="status", description="Status und Song anzeigen")
    async def status(interaction: discord.Interaction):
        if not await gate(interaction):
            return
        voice = bot.radio_voice()
        playing = bool(voice and voice.is_playing())
        try:
            timeout = aiohttp.ClientTimeout(total=8)
            async with aiohttp.ClientSession(timeout=timeout) as session:
                async with session.get(BASE + "/api/nowplaying/" + quote(STATION, safe="")) as response:
                    response.raise_for_status()
                    data = await response.json()
            song = (data.get("now_playing") or {}).get("song") or {}
            track = f"{song.get('artist') or 'Unbekannt'} – {song.get('title') or 'Unbekannt'}"
        except Exception:
            track = "Momentan nicht abrufbar"
        await interaction.response.send_message(
            f"📻 **BBQ-Chaos-Deutschland**\n"
            f"Discord: {'▶️ spielt' if playing else '⏸️ pausiert' if bot.paused_by_user else '⏳ nicht spielend'}\n"
            f"Lautstaerke: **{round(bot.volume * 100)} %**\n"
            f"🎵 **Aktuell:** {discord.utils.escape_markdown(track)[:300]}",
            ephemeral=True)

    @musik.command(name="suchen", description="Nach einem wuenschbaren Song suchen")
    @app_commands.describe(suchbegriff="Interpret oder Songtitel")
    async def search(interaction: discord.Interaction, suchbegriff: app_commands.Range[str, 2, 100]):
        if not await gate(interaction):
            return
        await interaction.response.defer(ephemeral=True, thinking=True)
        try:
            response = await request_api("GET", "/requests", params={"search": suchbegriff, "per_page": 50})
            results = []
            for item in entries(response):
                if not isinstance(item, dict):
                    continue
                artist, title, identity = describe(item)
                if not identity or (suchbegriff.casefold() not in (artist + " " + title).casefold()):
                    continue
                results.append(f"🎵 **{discord.utils.escape_markdown(artist)} – {discord.utils.escape_markdown(title)}**\nID: `{identity}`")
                if len(results) == 8:
                    break
            message = "\n\n".join(results) if results else "Keine passenden wuenschbaren Titel gefunden."
            await interaction.followup.send(message[:1900], ephemeral=True)
        except Exception:
            log.exception("AzuraCast-Suche fehlgeschlagen")
            await interaction.followup.send("AzuraCast-Suche derzeit nicht verfuegbar. Bitte API und Songwuensche pruefen.", ephemeral=True)

    @musik.command(name="wuenschen", description="Song anhand seiner ID aus /musik suchen wuenschen")
    @app_commands.describe(song_id="ID aus dem Ergebnis von /musik suchen")
    async def request(interaction: discord.Interaction, song_id: str):
        if not await gate(interaction):
            return
        await interaction.response.defer(ephemeral=True, thinking=True)
        user_id = str(interaction.user.id)
        async with lock:
            remaining = COOLDOWN - (time.time() - cooldowns.get(user_id, 0))
            if remaining > 0:
                await interaction.followup.send(f"⏳ Bitte noch **{int(remaining + 59) // 60} Minuten** warten.", ephemeral=True)
                return
            try:
                if not song_id.isdigit():
                    await interaction.followup.send("Ungueltige Song-ID. Bitte /musik suchen verwenden.", ephemeral=True)
                    return
                result = await request_api("POST", "/request/" + quote(song_id, safe=""))
                if isinstance(result, dict) and result.get("success") is False:
                    await interaction.followup.send("AzuraCast hat den Musikwunsch abgelehnt.", ephemeral=True)
                    return
                cooldowns[user_id] = time.time()
                save_cooldowns()
                await interaction.followup.send("✅ Musikwunsch eingereicht! AzuraCast bestimmt den Wiedergabezeitpunkt.", ephemeral=True)
            except Exception:
                log.exception("Musikwunsch fehlgeschlagen")
                await interaction.followup.send("❌ Musikwunsch nicht angenommen. Bitte AzuraCast-Wunschfunktion pruefen.", ephemeral=True)

    bot.tree.add_command(radio)
    bot.tree.add_command(musik)
