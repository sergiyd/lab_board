
export class BufferReader {
	private readonly _dataView: DataView;
	private _position = 0;

	constructor(buffer: ArrayBuffer) {
		this._dataView = new DataView(buffer);
	}

	public readUint8(): number {
		return this._dataView.getUint8(this._position++);
	}

	public readUint32(): number {
		const result = this._dataView.getUint32(this._position, littleEndian);
		this._position += 4;

		return result;
	}

	public slice(length?: number): ArrayBuffer {
		const inFactLength = length ?? this._dataView.byteLength - this._position;
		const result = this._dataView.buffer.slice(this._position, this._position + inFactLength);
		this._position += inFactLength;

		return result;
	}

	public sliceSized(): ArrayBuffer {
		return this.slice(this.readUint8());
	}
}

const littleEndian = true;
