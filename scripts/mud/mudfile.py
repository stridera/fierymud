import os


class MudFile:
    @classmethod
    def from_filename(cls, filename: str):
        index = filename.endswith("index")
        path = filename[:-5] if index else filename

        file_list = []
        if index:
            for line in open(filename, encoding="ascii"):
                if line.startswith("$"):
                    break
                if line.strip() == "":
                    continue
                file_list.append(path + line.rstrip())
        else:
            file_list.append(filename)

        for file in file_list:
            yield cls(file)

    @classmethod
    def _player_files(cls, path: str, player: str):
        player_name = player[0].upper() + player[1:].lower()
        root = f"{path}/{player[0].upper()}/{player_name}"
        files = []
        if os.path.exists(root + ".plr"):
            files.append({"class": "Player", "filename": root + ".plr"})
        if os.path.exists(root + ".objs"):
            files.append({"class": "Objects", "filename": root + ".objs"})
        if os.path.exists(root + ".pets"):
            files.append({"class": "Pets", "filename": root + ".pets"})
        if os.path.exists(root + ".quests"):
            files.append({"class": "Quests", "filename": root + ".quests"})
        if os.path.exists(root + ".notes"):
            files.append({"class": "Notes", "filename": root + ".notes"})
        return {"name": player_name, "root": root, "files": files}

    @classmethod
    def player_files(cls, path: str, player: str):
        if player is None:
            for _root, _dirs, files in os.walk(path):
                for file in files:
                    if file.endswith(".plr"):
                        yield cls._player_files(path, file[:-4])
        else:
            yield cls._player_files(path, player)

    def __init__(self, filename: str):
        self.filename = filename
        self.data = []

    def __iter__(self):
        """Process a file"""
        with open(self.filename, "r", encoding="ascii") as f:
            for line in f:
                yield line.strip()

    def current_file(self):
        return self.filename
