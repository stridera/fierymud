from typing import List, Self


class BitFlags:
    """
    Class to handle bit flags
    """

    def __init__(self, flags: List[str]):
        """
        :param data: Data to parse
        :param flags: List of flags
        """

        self.ascii_flags = list(map(chr, range(97, 123))) + list(map(chr, range(65, 91)))
        self.bits_set = []
        self.flags = flags

    def reset(self) -> Self:
        self.bits_set = []
        return self

    def parse(self, data: str, offset: int = 0) -> Self:
        self.bits_set = []
        self.set_flags(data, offset)
        return self

    def set_flags(self, data: str, offset: int = 0) -> Self:
        """
        Sets the flags
        :param data: The data to parse
        :return: None
        """
        if data.lstrip("-").isdigit():  # Integer Flags
            int_flag = int(data.lstrip("-"))
            if data.startswith("-"):
                int_flag |= 1 << 32  # Assume a 32-bit integer

            int_flag = int(data)
            self.bits_set += [i + offset for i in range(32) if int_flag & (1 << i)]
        else:  # ASCII Flags
            self.bits_set += [self.ascii_flags.index(flag) + offset for flag in [*data]]

        self.bits_set.sort()
        return self

    def as_ascii(self):
        """
        :return: The flags as ascii
        """
        resp = "".join([self.ascii_flags[i] for i in self.bits_set])
        return resp if resp else "0"

    def __str__(self):
        """
        :return: String representation of the flags
        """
        result = []
        for i in self.bits_set:
            if i < len(self.flags):
                result.append(self.flags[i])
            else:
                result.append(f"UNKNOWN({i})")
        return ", ".join(result)

    def __repr__(self):
        """
        :return: String representation of the flags
        """
        return self.__str__()

    def __iter__(self):
        for i in self.bits_set:
            if i < len(self.flags):
                yield self.flags[i]
            else:
                print(f"UNKNOWN({i}) (max: {len(self.flags)}) in {self.flags}")

    def json_repr(self):
        return [flag for flag in list(self) if flag is not None]

    @staticmethod
    def read_flag_list(data: str, flags: list[str]) -> list[str]:
        """
        Reads a list of flags from the data
        :param data: The data to read from
        :return: The list of flags read
        """
        data_lst = data.split(" ")
        if len(data_lst) == 0:
            return BitFlags(flags)

        bitflags = BitFlags(flags)
        for i, flag in enumerate(data_lst):
            bitflags.set_flags(flag, 32 * i)
        return list(bitflags)

    @staticmethod
    def read_flags(data: str, flags: list[str], offset: int = 0) -> Self:
        """
        Reads a string of flags from the data
        :param data: The data to read from
        :return: The flags read
        """
        return BitFlags(flags).set_flags(data, offset)

    @staticmethod
    def build_flags(flag, flag_list, offset=0) -> list[str]:
        """Builds a string of flags from a bitfield"""
        active = []
        flag = int(flag)
        if flag.bit_length() > len(flag_list):
            raise ValueError(
                f"Flag out of range! {flag}({bin(flag)}:{flag.bit_length()} bits = {len(flag_list)} ({flag_list})"
            )
        for i in range(len(flag_list)):
            if flag & (1 << i + offset):
                active.append(flag_list[i])
        return active

    @staticmethod
    def get_flag(flag: int, flag_list: list[str]) -> str:
        """Gets a flag from a list"""
        flag = int(flag)
        if flag >= len(flag_list):
            raise ValueError(f"Flag out of range! {flag} >= {len(flag_list)} ({flag_list})")
        return flag_list[flag]


if __name__ == "__main__":
    # Test the class
    flags = [f"flag{i}" for i in range(32)]

    print("Testing...")

    bf = BitFlags(flags)

    # Integers
    assert str(bf.parse("0")) == "", bf.parse("0")
    assert str(bf.parse("1")) == "flag0", bf.parse("1")
    assert str(bf.parse("2")) == "flag1", bf.parse("2")
    assert str(bf.parse("3")) == "flag0, flag1", bf.parse("3")
    assert str(bf.parse("4")) == "flag2", bf.parse("4")
    assert str(bf.parse("5")) == "flag0, flag2", bf.parse("5")
    assert str(bf.parse("6")) == "flag1, flag2", bf.parse("6")
    assert bf.parse("-2147358694").as_ascii() == "bdelnopqF", bf.parse("-2147358694").as_ascii()

    # ASCII
    assert str(bf.parse("")) == "", bf.parse("0")
    assert str(bf.parse("a")) == "flag0", bf.parse("a")
    assert (
        str(bf.parse("bdelnopqF")) == "flag1, flag3, flag4, flag11, flag13, flag14, flag15, flag16, flag31"
    ), bf.parse("bdelnopqF")

    # Append
    assert str(bf.parse("2").set_flags("1")) == "flag0, flag1", bf.parse("2").set_flags("1")

    print("All tests passed!")
